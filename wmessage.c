#include "pbc.h"
#include "context.h"
#include "alloc.h"
#include "varint.h"
#include "map.h"
#include "proto.h"

#include <stdint.h>
#include <string.h>

#define WMESSAGE_SIZE 64

struct pbc_wmessage {
	struct _message *type;
	uint8_t * buffer;
	uint8_t * ptr;
	uint8_t * endptr;
	pbc_array sub;
};

static struct pbc_wmessage *
_wmessage_new(struct _message *msg) {
	struct pbc_wmessage * m = malloc(sizeof(*m));
	m->type = msg;
	m->buffer = malloc(WMESSAGE_SIZE);
	m->ptr = m->buffer;
	m->endptr = m->buffer + WMESSAGE_SIZE;
	pbc_array_open(m->sub);

	return m;
}

struct pbc_wmessage * 
pbc_wmessage_new(struct pbc_env * env, const char *typename) {
	struct _message * msg = proto_get_message(env, typename);
	if (msg == NULL)
		return NULL;
	return _wmessage_new(msg);
}

void 
pbc_wmessage_delete(struct pbc_wmessage *m) {
	int sz = pbc_array_size(m->sub);
	int i;
	for (i=0;i<sz;i++) {
		pbc_var var;
		pbc_array_index(m->sub,i,var);
		pbc_wmessage_delete(var->p[0]);
	}
	pbc_array_close(m->sub);
	free(m->buffer);
	free(m);
}

static void
_expand(struct pbc_wmessage *m, int sz) {
	if (m->ptr + sz > m->endptr) {
		int cap = m->endptr - m->buffer;
		sz = m->ptr + sz - m->buffer;
		do {
			cap = cap * 2;
		} while ( sz <= cap ) ;
		uint8_t * buffer = realloc(m->buffer, cap);
		m->ptr = buffer + (m->ptr - m->buffer);
		m->endptr = buffer + cap;
		m->buffer = buffer;
	}
}

void 
pbc_wmessage_integer(struct pbc_wmessage *m, const char *key, uint32_t low, uint32_t hi) {
	struct _field * f = _pbcM_sp_query(m->type->name,key);
	if (f==NULL) {
		// todo : error
		return;
	}
	if (f->label != LABEL_REPEATED) {
		if (f->type == PTYPE_ENUM) {
			if (low == f->default_v->e.id)
				return;
		} else {
			if (low == f->default_v->integer.low &&
				hi == f->default_v->integer.hi) {
				return;
			}
		}
	}
	int id = f->id << 3;

	_expand(m,10);
	switch (f->type) {
	case PTYPE_INT64:
	case PTYPE_UINT64: 
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,10);
		m->ptr += varint_encode((uint64_t)low | (uint64_t)hi << 32 , m->ptr);
		break;
	case PTYPE_INT32:
	case PTYPE_UINT32:
	case PTYPE_ENUM:
	case PTYPE_BOOL:
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,10);
		m->ptr += varint_encode32(low, m->ptr);
		break;
	case PTYPE_FIXED64:
	case PTYPE_SFIXED64:
		id |= WT_BIT64;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,8);
		m->ptr[0] = (uint8_t)(low & 0xff);
		m->ptr[1] = (uint8_t)(low >> 8 & 0xff);
		m->ptr[2] = (uint8_t)(low >> 16 & 0xff);
		m->ptr[3] = (uint8_t)(low >> 24 & 0xff);
		m->ptr[4] = (uint8_t)(hi & 0xff);
		m->ptr[5] = (uint8_t)(hi >> 8 & 0xff);
		m->ptr[6] = (uint8_t)(hi >> 16 & 0xff);
		m->ptr[7] = (uint8_t)(hi >> 24 & 0xff);
		break;
	case PTYPE_FIXED32:
	case PTYPE_SFIXED32:
		id |= WT_BIT32;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,4);
		m->ptr[0] = (uint8_t)(low & 0xff);
		m->ptr[1] = (uint8_t)(low >> 8 & 0xff);
		m->ptr[2] = (uint8_t)(low >> 16 & 0xff);
		m->ptr[3] = (uint8_t)(low >> 24 & 0xff);
		break;
	case PTYPE_SINT32:
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,10);
		m->ptr += varint_zigzag32(low, m->ptr);
		break;
	case PTYPE_SINT64:
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,10);
		m->ptr += varint_zigzag((uint64_t)low | (uint64_t)hi << 32 , m->ptr);
		break;
	}
}

static inline void
float_encode(float v , uint8_t * buffer) {
	union {
		float v;
		uint32_t e;
	} u;
	u.v = v;
	buffer[0] = (uint8_t) (u.e & 0xff);
	buffer[1] = (uint8_t) (u.e >> 8 & 0xff);
	buffer[2] = (uint8_t) (u.e >> 16 & 0xff);
	buffer[3] = (uint8_t) (u.e >> 24 & 0xff);
}

static inline void
double_encode(double v , uint8_t * buffer) {
	union {
		float v;
		uint64_t e;
	} u;
	u.v = v;
	buffer[0] = (uint8_t) (u.e & 0xff);
	buffer[1] = (uint8_t) (u.e >> 8 & 0xff);
	buffer[2] = (uint8_t) (u.e >> 16 & 0xff);
	buffer[3] = (uint8_t) (u.e >> 24 & 0xff);
	buffer[4] = (uint8_t) (u.e >> 32 & 0xff);
	buffer[5] = (uint8_t) (u.e >> 40 & 0xff);
	buffer[6] = (uint8_t) (u.e >> 48 & 0xff);
	buffer[7] = (uint8_t) (u.e >> 56 & 0xff);
}

void 
pbc_wmessage_real(struct pbc_wmessage *m, const char *key, double v) {
	struct _field * f = _pbcM_sp_query(m->type->name,key);
	if (f == NULL) {
		// todo : error
		return;
	}
	if (f->label != LABEL_REPEATED) {
		if (v == f->default_v->real)
			return;
	}
	int id = f->id << 3;
	_expand(m,10);
	switch (f->type) {
	case PTYPE_FLOAT: {
		id |= WT_BIT32;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,4);
		float_encode(v , m->ptr);
		m->ptr += 4;
		break;
	}
	case PTYPE_DOUBLE:
		id |= WT_BIT32;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,8);
		double_encode(v , m->ptr);
		m->ptr += 8;
		break;
	}
}

void 
pbc_wmessage_string(struct pbc_wmessage *m, const char *key, const char * v, int len) {
	struct _field * f = _pbcM_sp_query(m->type->name,key);
	if (f == NULL) {
		// todo : error
		return;
	}
	if (len == 0) {
		len = strlen(v);
	}
	if (f->label != LABEL_REPEATED) {
		if (f->type == PTYPE_ENUM) {
			if (strncmp(v , f->default_v->e.name, len) == 0 && f->default_v->e.name[len] =='\0') {
				return;
			}
		} else if (f->type == PTYPE_STRING) {
			if (len == f->default_v->s.len &&
				strcmp(v, f->default_v->s.str) == 0) {
				return;
			}
		}
	}
	int id = f->id << 3;
	_expand(m,10);
	switch (f->type) {
	case PTYPE_ENUM : {
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		char temp[len+1];
		if (v[len] != '\0') {
			memcpy(temp,v,len);
			temp[len]='\0';
			v = temp;
		}
		_expand(m,10);
		int enum_id = _pbcM_si_query(f->type_name.e->name, v);
		m->ptr += varint_encode32(enum_id, m->ptr);
		break;
	}
	case PTYPE_STRING:
	case PTYPE_BYTES:
		id |= WT_LEND;
		m->ptr += varint_encode32(id, m->ptr);
		_expand(m,10);
		m->ptr += varint_encode32(len, m->ptr);
		_expand(m,len);
		memcpy(m->ptr , v , len);
		m->ptr += len;
		break;
	}
}

struct pbc_wmessage * 
pbc_wmessage_message(struct pbc_wmessage *m, const char *key) {
	struct _field * f = _pbcM_sp_query(m->type->name,key);
	if (f == NULL) {
		// todo : error
		return NULL;
	}
	pbc_var var;
	var->p[0] = _wmessage_new(f->type_name.m);
	var->p[1] = f;
	pbc_array_push(m->sub , var);
	return var->p[0];
}

void * 
pbc_wmessage_buffer(struct pbc_wmessage *m, int *sz) {
	if (m->ptr == m->buffer)
		return NULL;
	*sz = m->ptr - m->buffer;
	int i;
	int n = pbc_array_size(m->sub);
	for (i=0;i<n;i++) {
		pbc_var var;
		pbc_array_index(m->sub, i , var);
		int size;
		void * buf = pbc_wmessage_buffer(var->p[0] , &size);
		if (buf) {
			struct _field * f = var->p[1];
			int id = f->id << 3 | WT_LEND;
			_expand(m,10);
			m->ptr += varint_encode32(id, m->ptr);
			_expand(m,10);
			m->ptr += varint_encode32(size, m->ptr);
			_expand(m,size);
			memcpy(m->ptr, buf,size);
			m->ptr += size;
		}
	}
	return m->ptr;
}

