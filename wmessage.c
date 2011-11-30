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
	struct map_sp *packed;
};

struct _packed {
	int id;
	int ptype;
	pbc_array data;
};

static struct pbc_wmessage *
_wmessage_new(struct _message *msg) {
	struct pbc_wmessage * m = malloc(sizeof(*m));
	m->type = msg;
	m->buffer = malloc(WMESSAGE_SIZE);
	m->ptr = m->buffer;
	m->endptr = m->buffer + WMESSAGE_SIZE;
	_pbcA_open(m->sub);
	m->packed = NULL;

	return m;
}

struct pbc_wmessage * 
pbc_wmessage_new(struct pbc_env * env, const char *typename) {
	struct _message * msg = _pbcP_get_message(env, typename);
	if (msg == NULL)
		return NULL;
	return _wmessage_new(msg);
}

static void
_free_packed(void *p) {
	struct _packed * packed = p;
	_pbcA_close(packed->data);
	free(p);
}

void 
pbc_wmessage_delete(struct pbc_wmessage *m) {
	int sz = pbc_array_size(m->sub);
	int i;
	for (i=0;i<sz;i++) {
		pbc_var var;
		_pbcA_index(m->sub,i,var);
		pbc_wmessage_delete(var->p[0]);
	}
	_pbcA_close(m->sub);

	if (m->packed) {
		_pbcM_sp_foreach(m->packed, _free_packed);
		_pbcM_sp_delete(m->packed);
	}

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
		} while ( sz > cap ) ;
		uint8_t * buffer = realloc(m->buffer, cap);
		m->ptr = buffer + (m->ptr - m->buffer);
		m->endptr = buffer + cap;
		m->buffer = buffer;
	}
}

static struct _packed *
_get_packed(struct pbc_wmessage *m , struct _field *f , const char *key) {
	if (m->packed == NULL) {
		m->packed = _pbcM_sp_new();
	}
	void ** v = _pbcM_sp_query_insert(m->packed , key);
	if (*v == NULL) {
		*v = malloc(sizeof(struct _packed));
		struct _packed *p = *v;
		p->id = f->id;
		p->ptype = f->type;
		_pbcA_open(p->data);
		return p;
	}
	return *v;
}

static void
_packed_integer(struct pbc_wmessage *m, struct _field *f, const char *key , uint32_t low, uint32_t hi) {
	struct _packed * packed = _get_packed(m,f,key);
	pbc_var var;
	var->integer.low = low;
	var->integer.hi = hi;
	_pbcA_push(packed->data , var);
}

static void
_packed_real(struct pbc_wmessage *m, struct _field *f, const char *key , double v) {
	struct _packed * packed = _get_packed(m,f,key);
	pbc_var var;
	var->real = v;
	_pbcA_push(packed->data , var);
}

static inline void
int64_encode(uint32_t low, uint32_t hi , uint8_t * buffer) {
	buffer[0] = (uint8_t)(low & 0xff);
	buffer[1] = (uint8_t)(low >> 8 & 0xff);
	buffer[2] = (uint8_t)(low >> 16 & 0xff);
	buffer[3] = (uint8_t)(low >> 24 & 0xff);
	buffer[4] = (uint8_t)(hi & 0xff);
	buffer[5] = (uint8_t)(hi >> 8 & 0xff);
	buffer[6] = (uint8_t)(hi >> 16 & 0xff);
	buffer[7] = (uint8_t)(hi >> 24 & 0xff);
}

static inline void
int32_encode(uint32_t low, uint8_t * buffer) {
	buffer[0] = (uint8_t)(low & 0xff);
	buffer[1] = (uint8_t)(low >> 8 & 0xff);
	buffer[2] = (uint8_t)(low >> 16 & 0xff);
	buffer[3] = (uint8_t)(low >> 24 & 0xff);
}

void 
pbc_wmessage_integer(struct pbc_wmessage *m, const char *key, uint32_t low, uint32_t hi) {
	struct _field * f = _pbcM_sp_query(m->type->name,key);
	if (f==NULL) {
		// todo : error
		return;
	}
	if (f->label == LABEL_PACKED) {
		_packed_integer(m , f, key , low, hi);
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

	_expand(m,20);
	switch (f->type) {
	case PTYPE_INT64:
	case PTYPE_UINT64: 
	case PTYPE_INT32:
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		m->ptr += varint_encode((uint64_t)low | (uint64_t)hi << 32 , m->ptr);
		break;
	case PTYPE_UINT32:
	case PTYPE_ENUM:
	case PTYPE_BOOL:
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		m->ptr += varint_encode32(low, m->ptr);
		break;
	case PTYPE_FIXED64:
	case PTYPE_SFIXED64:
		id |= WT_BIT64;
		m->ptr += varint_encode32(id, m->ptr);
		int64_encode(low,hi,m->ptr);
		break;
	case PTYPE_FIXED32:
	case PTYPE_SFIXED32:
		id |= WT_BIT32;
		m->ptr += varint_encode32(id, m->ptr);
		int32_encode(low,m->ptr);
		break;
	case PTYPE_SINT32:
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		m->ptr += varint_zigzag32(low, m->ptr);
		break;
	case PTYPE_SINT64:
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		m->ptr += varint_zigzag((uint64_t)low | (uint64_t)hi << 32 , m->ptr);
		break;
	}
}

void 
pbc_wmessage_real(struct pbc_wmessage *m, const char *key, double v) {
	struct _field * f = _pbcM_sp_query(m->type->name,key);
	if (f == NULL) {
		// todo : error
		return;
	}
	if (f->label == LABEL_PACKED) {
		_packed_real(m , f, key , v);
		return;		
	}

	if (f->label != LABEL_REPEATED) {
		if (v == f->default_v->real)
			return;
	}
	int id = f->id << 3;
	_expand(m,18);
	switch (f->type) {
	case PTYPE_FLOAT: {
		id |= WT_BIT32;
		m->ptr += varint_encode32(id, m->ptr);
		float_encode(v , m->ptr);
		m->ptr += 4;
		break;
	}
	case PTYPE_DOUBLE:
		id |= WT_BIT32;
		m->ptr += varint_encode32(id, m->ptr);
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
	} else if (len<0) {
		// -1 for add '\0'
		len = strlen(v) - len;
	}

	if (f->label == LABEL_PACKED) {
		if (f->type == PTYPE_ENUM) {
			char temp[len+1];
			if (v[len] != '\0') {
				memcpy(temp,v,len);
				temp[len]='\0';
				v = temp;
			}
			int enum_id = _pbcM_si_query(f->type_name.e->name, v);
			if (enum_id < 0) {
				// todo : error , invalid enum
				return;
			}
			_packed_integer(m , f, key , enum_id , 0);
		}
		return;	
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
	_expand(m,20);
	switch (f->type) {
	case PTYPE_ENUM : {
		char temp[len+1];
		if (v[len] != '\0') {
			memcpy(temp,v,len);
			temp[len]='\0';
			v = temp;
		}
		int enum_id = _pbcM_si_query(f->type_name.e->name, v);
		if (enum_id < 0) {
			// todo : error , enum invalid
			return;
		}
		id |= WT_VARINT;
		m->ptr += varint_encode32(id, m->ptr);
		m->ptr += varint_encode32(enum_id, m->ptr);
		break;
	}
	case PTYPE_STRING:
	case PTYPE_BYTES:
		id |= WT_LEND;
		m->ptr += varint_encode32(id, m->ptr);
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
	_pbcA_push(m->sub , var);
	return var->p[0];
}

static void
_pack_packed_64(struct _packed *p,struct pbc_wmessage *m) {
	int n = pbc_array_size(p->data);
	int len = n * 8;
	int i;
	pbc_var var;
	_expand(m,10 + len);
	m->ptr += varint_encode32(len, m->ptr);
	switch (p->ptype) {
	case PTYPE_DOUBLE:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			double_encode(var->real , m->ptr + i * 8);
		}
		break;
	default:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			int64_encode(var->integer.low , var->integer.hi, m->ptr + i * 8);
		}
		break;
	}
	m->ptr += len;
}

static void
_pack_packed_32(struct _packed *p,struct pbc_wmessage *m) {
	int n = pbc_array_size(p->data);
	int len = n * 4;
	int i;
	pbc_var var;
	_expand(m,10 + len);
	m->ptr += varint_encode32(len, m->ptr);
	switch (p->ptype) {
	case PTYPE_FLOAT:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			float_encode(var->real , m->ptr + i * 8);
		}
		break;
	default:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			int32_encode(var->integer.low , m->ptr + i * 8);
		}
		break;
	}
	m->ptr += len;
}

static void
_pack_packed_varint(struct _packed *p,struct pbc_wmessage *m) {
	int n = pbc_array_size(p->data);
	int offset = m->ptr - m->buffer;
	int len = n * 2;
	if (p->ptype == PTYPE_BOOL) {
		len = n;
	}
	int i;
	pbc_var var;
	_expand(m,10 + len);
	int len_len = varint_encode32(len, m->ptr);
	m->ptr += len_len;

	switch (p->ptype) {
	case PTYPE_INT64:
	case PTYPE_UINT64:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			_expand(m,10);
			m->ptr += varint_encode((uint64_t)var->integer.low | (uint64_t)var->integer.hi << 32 , m->ptr);
		}
		break;
	case PTYPE_INT32:
	case PTYPE_BOOL:
	case PTYPE_UINT32:
	case PTYPE_ENUM:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			_expand(m,10);
			m->ptr += varint_encode32(var->integer.low , m->ptr);
		}
		break;
	case PTYPE_SINT32:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			_expand(m,10);
			m->ptr += varint_zigzag32(var->integer.low, m->ptr);
		}
		break;
	case PTYPE_SINT64:
		for (i=0;i<n;i++) {
			_pbcA_index(p->data, i, var);
			_expand(m,10);
			m->ptr += varint_zigzag((uint64_t)var->integer.low | (uint64_t)var->integer.hi << 32 , m->ptr);
		}
		break;
	default:
		// error
		m->ptr += len;
		break;
	}
	int end_offset = m->ptr - m->buffer;
	int end_len = end_offset - (offset + len_len);
	if (end_len != len) {
		uint8_t temp[10];
		int end_len_len = varint_encode32(end_len, temp);
		if (end_len_len != len_len) {
			_expand(m, end_len_len);
			memmove(m->buffer + offset + end_len_len , 
				m->buffer + offset + len_len , 
				end_len);
		}
		memcpy(m->buffer + offset , temp, end_len_len);
	}
}

static void
_pack_packed(void *p, void *ud) {
	struct _packed *packed = p;
	struct pbc_wmessage * m = ud;
	int id = packed->id << 3 | WT_LEND;
	_expand(m,10);
	m->ptr += varint_encode32(id, m->ptr);
	switch(packed->ptype) {
	case PTYPE_DOUBLE:
	case PTYPE_FIXED64:
	case PTYPE_SFIXED64:
		_pack_packed_64(packed,m);
		break;
	case PTYPE_FLOAT:
	case PTYPE_FIXED32:
	case PTYPE_SFIXED32:
		_pack_packed_32(packed,m);
		break;
	default:
		_pack_packed_varint(packed,m);
		break;
	}
}

void * 
pbc_wmessage_buffer(struct pbc_wmessage *m, struct pbc_slice *slice) {
	if (m->packed) {
		_pbcM_sp_foreach_ud(m->packed , _pack_packed, m);
	}
	int i;
	int n = pbc_array_size(m->sub);
	for (i=0;i<n;i++) {
		pbc_var var;
		_pbcA_index(m->sub, i , var);
		struct pbc_slice s;
		pbc_wmessage_buffer(var->p[0] , &s);
		if (s.buffer) {
			struct _field * f = var->p[1];
			int id = f->id << 3 | WT_LEND;
			_expand(m,20+s.len);
			m->ptr += varint_encode32(id, m->ptr);
			m->ptr += varint_encode32(s.len, m->ptr);
			memcpy(m->ptr, s.buffer, s.len);
			m->ptr += s.len;
		}
	}
	slice->buffer = m->buffer;
	slice->len = m->ptr - m->buffer;

	return m->buffer;
}

