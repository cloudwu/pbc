#include "pbc.h"
#include "alloc.h"
#include "map.h"
#include "context.h"
#include "proto.h"
#include "pattern.h"
#include "varint.h"

#include <stddef.h>
#include <string.h>

struct pbc_rmessage {
		struct _message * msg;
		struct map_sp * index;	// key -> struct value *
};

union _var {
	pbc_var var;
	pbc_array array;
	struct pbc_rmessage message;
} ;

struct value {
	struct _field * type;
	union _var v;
};

#define SIZE_VAR (offsetof(struct value, v) + sizeof(pbc_var))
#define SIZE_ARRAY (offsetof(struct value, v) + sizeof(pbc_array))
#define SIZE_MESSAGE (offsetof(struct value, v) + sizeof(struct pbc_rmessage))

static struct value *
read_string(struct atom *a,struct _field *f, uint8_t *buffer) {
	const char * temp = (const char *) (buffer + a->v.s.start);
	int len = a->v.s.end - a->v.s.start;

	if (temp[len] == '\0') {
		struct value * v = malloc(SIZE_VAR);
		v->v.var->s.str = temp;
		v->v.var->s.len = len;
		return v;
	} else {
		struct value * v = malloc(SIZE_VAR + len + 1);
		memcpy(((char *)v) + SIZE_VAR , temp, len);
		*(((char *)v) + SIZE_VAR + len) = '\0';
		v->v.var->s.str = ((char *)v) + SIZE_VAR;
		v->v.var->s.len = len;
		return v;
	}
}

static void
read_string_var(pbc_var var,struct atom *a,struct _field *f,uint8_t *buffer) {
	const char * temp = (const char *) (buffer + a->v.s.start);
	int len = a->v.s.end - a->v.s.start;
	if (temp[len] == '\0') {
		var->s.str = temp;
		var->s.len = len;
	} else if (len == 0) {
			var->s.str = "";
			var->s.len = 0;
	} else {
		char * temp2 = malloc(len + 1);
		memcpy(temp2, temp, len);
		temp2[len]='\0';
		var->s.str = temp2;
		var->s.len = -len;
	}
}

static void _pbc_rmessage_new(struct pbc_rmessage * ret , struct _message * type ,  void *buffer, int size);

static struct value *
read_value(struct _field *f, struct atom * a, uint8_t *buffer) {
	struct value * v;

	switch (f->type) {
	case PTYPE_DOUBLE:
		v = malloc(SIZE_VAR);
		v->v.var->real = read_double(a);
		break;
	case PTYPE_FLOAT:
		v = malloc(SIZE_VAR);
		v->v.var->real = (double) read_float(a);
		break;
	case PTYPE_ENUM:
		v = malloc(SIZE_VAR);
		v->v.var->e.id = a->v.i.low;
		v->v.var->e.name = _pbcM_ip_query(f->type_name.e->id , a->v.i.low);
		break;
	case PTYPE_INT64:
	case PTYPE_UINT64:
	case PTYPE_INT32:
	case PTYPE_UINT32:
	case PTYPE_FIXED32:
	case PTYPE_FIXED64:
	case PTYPE_SFIXED32:
	case PTYPE_SFIXED64:
	case PTYPE_BOOL:
		v = malloc(SIZE_VAR);
		v->v.var->integer = a->v.i;
		break;
	case PTYPE_SINT32: 
		v = malloc(SIZE_VAR);
		v->v.var->integer = a->v.i;
		varint_dezigzag32(&(v->v.var->integer));
		break;
	case PTYPE_SINT64:
		v = malloc(SIZE_VAR);
		v->v.var->integer = a->v.i;
		varint_dezigzag64(&(v->v.var->integer));
		break;
	case PTYPE_STRING:
		v = read_string(a,f,buffer);
		break;
	case PTYPE_BYTES:
		v = malloc(SIZE_VAR);
		v->v.var->s.str = (const char *)(buffer + a->v.s.start);
		v->v.var->s.len = a->v.s.end - a->v.s.start;
		break;
	case PTYPE_MESSAGE:
		v = malloc(SIZE_MESSAGE);
		_pbc_rmessage_new(&(v->v.message), f->type_name.m , 
			buffer + a->v.s.start , 
			a->v.s.end - a->v.s.start);
		break;
	default:
		return NULL;
	}
	v->type = f;
	return v;
}

static void
push_value_packed(pbc_array array, struct _field *f, struct atom * aa, uint8_t *buffer) {
	pbc_ctx packed;
	struct context * ctx = (struct context *)packed;
	int n = _pbcC_open_packed(packed , f->type, 
			(uint8_t *)buffer + aa->v.s.start, aa->v.s.end - aa->v.s.start);
	if (n<=0) {
		// todo  : error
		return;
	}
	int i;
	pbc_var v;

	switch(f->type) {
	case PTYPE_DOUBLE:
		for (i=0;i<n;i++) {
			v->real = read_double(&ctx->a[i]);
			_pbcA_push(array,v);
		}
		break;
	case PTYPE_FLOAT:
		for (i=0;i<n;i++) {
			v->real = read_float(&ctx->a[i]);
			_pbcA_push(array,v);
		}
		break;
	case PTYPE_ENUM:
		for (i=0;i<n;i++) {
			struct atom *a = &ctx->a[i];
			v->e.id = a->v.i.low;
			v->e.name = _pbcM_ip_query(f->type_name.e->id , a->v.i.low);
			_pbcA_push(array,v);
		}
		break;
	case PTYPE_INT64:
	case PTYPE_UINT64:
	case PTYPE_INT32:
	case PTYPE_UINT32:
	case PTYPE_FIXED32:
	case PTYPE_FIXED64:
	case PTYPE_SFIXED32:
	case PTYPE_SFIXED64:
	case PTYPE_BOOL:
		for (i=0;i<n;i++) {
			struct atom *a = &ctx->a[i];
			v->integer = a->v.i;
			_pbcA_push(array,v);
		}
		break;
	case PTYPE_SINT32: 
		for (i=0;i<n;i++) {
			struct atom *a = &ctx->a[i];
			v->integer = a->v.i;
			varint_dezigzag32(&(v->integer));
			_pbcA_push(array,v);
		}
		break;
	case PTYPE_SINT64:
		for (i=0;i<n;i++) {
			struct atom *a = &ctx->a[i];
			v->integer = a->v.i;
			varint_dezigzag64(&(v->integer));
			_pbcA_push(array,v);
		}
		break;
	}
}

static void
push_value_array(pbc_array array, struct _field *f, struct atom * a, uint8_t *buffer) {
	pbc_var v;

	switch (f->type) {
	case PTYPE_DOUBLE:
		v->real = read_double(a);
		break;
	case PTYPE_FLOAT:
		v->real = (double) read_float(a);
		break;
	case PTYPE_ENUM:
		v->e.id = a->v.i.low;
		v->e.name = _pbcM_ip_query(f->type_name.e->id , a->v.i.low);
		break;
	case PTYPE_INT64:
	case PTYPE_UINT64:
	case PTYPE_INT32:
	case PTYPE_UINT32:
	case PTYPE_FIXED32:
	case PTYPE_FIXED64:
	case PTYPE_SFIXED32:
	case PTYPE_SFIXED64:
	case PTYPE_BOOL:
		v->integer = a->v.i;
		break;
	case PTYPE_SINT32: 
		v->integer = a->v.i;
		varint_dezigzag32(&(v->integer));
		break;
	case PTYPE_SINT64:
		v->integer = a->v.i;
		varint_dezigzag64(&(v->integer));
		break;
	case PTYPE_STRING:
		read_string_var(v,a,f,buffer);
		break;
	case PTYPE_BYTES:
		v->s.str = (const char *)(buffer + a->v.s.start);
		v->s.len = a->v.s.end - a->v.s.start;
		break;
	case PTYPE_MESSAGE: {
		struct pbc_rmessage message;
		_pbc_rmessage_new(&message, f->type_name.m , 
			buffer + a->v.s.start , 
			a->v.s.end - a->v.s.start);
		if (message.msg == NULL)
			return;
		v->p[0] = message.msg;
		v->p[1] = message.index;
		break;
	}
	default:
		return;
	}

	_pbcA_push(array,v);
}

static void
_pbc_rmessage_new(struct pbc_rmessage * ret , struct _message * type , void *buffer, int size) {
	pbc_ctx _ctx;
	if (_pbcC_open(_ctx,buffer,size) <=0) {
		memset(ret , 0, sizeof(*ret));
		return;
	}
	struct context * ctx = (struct context *)_ctx;

	ret->msg = type;
	ret->index = _pbcM_sp_new();

	int i;

	for (i=0;i<ctx->number;i++) {
		int id = ctx->a[i].id;
		struct _field * f = _pbcM_ip_query(type->id , id);
		if (f) {
			if (f->label == LABEL_REPEATED || f->label == LABEL_PACKED) {
				struct value * v;
				void ** vv = _pbcM_sp_query_insert(ret->index, f->name);
				if (*vv == NULL) {
					v = malloc(SIZE_ARRAY);
					v->type = f;
					_pbcA_open(v->v.array);
					*vv = v;
				} else {
					v= *vv;
				}
				if (f->label == LABEL_PACKED) {
					push_value_packed(v->v.array , f , &(ctx->a[i]), buffer);
				} else {
					push_value_array(v->v.array , f, &(ctx->a[i]), buffer);
				}
			} else {
				struct value * v = read_value(f, &(ctx->a[i]), buffer);
				if (v) {
					_pbcM_sp_insert(ret->index, f->name, v);
				}
			}
		}
	}

	_pbcC_close(_ctx);
}

struct pbc_rmessage * 
pbc_rmessage_new(struct pbc_env * env, const char * typename ,  struct pbc_slice * slice) {
	struct _message * msg = _pbcP_get_message(env, typename);
	if (msg == NULL)
		return NULL;
	struct pbc_rmessage temp;
	_pbc_rmessage_new(&temp, msg , slice->buffer, slice->len);
	if (temp.msg == NULL)
		return NULL;

	struct pbc_rmessage *m = malloc(sizeof(*m));
	*m = temp;
	return m;
}

static void _free_message(struct pbc_rmessage * m);

static void
_free_array(pbc_array array , int type) {
	if (type == PTYPE_MESSAGE) {
		int i;
		int sz = pbc_array_size(array);
		for (i=0;i<sz;i++) {
			pbc_var var;
			_pbcA_index(array,i,var);
			_free_message((struct pbc_rmessage *)(var->p));
		}
	} else if (type == PTYPE_STRING) {
		int i;
		int sz = pbc_array_size(array);
		for (i=0;i<sz;i++) {
			pbc_var var;
			_pbcA_index(array,i,var);
			if (var->s.len < 0) {
				free((void *)(var->s.str));
			}
		}
	}
	_pbcA_close(array);
}

static void
_free_value(void *p) {
	struct value *v = p;
	if (v->type->label == LABEL_REPEATED) {
		_free_array(v->v.array , v->type->type);
	} else if (v->type->type == PTYPE_MESSAGE) {
		_free_message(&(v->v.message));
	}
	free(v);
}

static void
_free_message(struct pbc_rmessage *m) {
	if (m->index) {
		_pbcM_sp_foreach(m->index, _free_value);
		_pbcM_sp_delete(m->index);
	}
}

void 
pbc_rmessage_delete(struct pbc_rmessage * m) {
	if (m) {
		_free_message(m);
		free(m);
	}
}

const char * 
pbc_rmessage_string(struct pbc_rmessage * m , const char *key , int index, int *sz) {
	struct value * v = _pbcM_sp_query(m->index,key);
	int type = 0;
	pbc_var var;
	if (v == NULL) {
		type = _pbcP_message_default(m->msg, key, var);
	} else {
		if (v->type->label == LABEL_REPEATED || v->type->label == LABEL_PACKED) {
			_pbcA_index(v->v.array, index, var);
		} else {
			var[0] = v->v.var[0];
		}
		type = v->type->type;
	}

	if (type == PTYPE_ENUM) {
		if (sz) {
			*sz = strlen(var->e.name);
		}
		return var->e.name;
	}

	if (sz) {
		int len = var->s.len;
		if (len<0) {
			len = - len;
		}
		*sz = len;
	}
	return var->s.str;
}

uint32_t 
pbc_rmessage_integer(struct pbc_rmessage *m , const char *key , int index, uint32_t *hi) {
	struct value * v = _pbcM_sp_query(m->index,key);
	pbc_var var;
	int type = 0;
	if (v == NULL) {
		type = _pbcP_message_default(m->msg, key, var);
	} else {
		if (v->type->label == LABEL_REPEATED || v->type->label == LABEL_PACKED) {
			_pbcA_index(v->v.array, index, var);
		} else {
			var[0] = v->v.var[0];
		}
		type = v->type->type;
	}

	if (type == PTYPE_ENUM) {
		if (hi) {
			*hi = 0;
		}
		return var->e.id;
	}

	if (hi) {
		*hi = var->integer.hi;
	}
	return var->integer.low;
}

double 
pbc_rmessage_real(struct pbc_rmessage * m, const char *key , int index) {
	struct value * v = _pbcM_sp_query(m->index,key);
	pbc_var var;
	if (v == NULL) {
		_pbcP_message_default(m->msg, key, var);
	} else {
		if (v->type->label == LABEL_REPEATED || v->type->label == LABEL_PACKED) {
			_pbcA_index(v->v.array, index, var);
		} else {
			return v->v.var->real;
		}
	}
	return var->real;
}


struct pbc_rmessage * 
pbc_rmessage_message(struct pbc_rmessage * rm, const char *key, int index) {
	struct value * v = _pbcM_sp_query(rm->index,key);
	if (v == NULL) {
		struct _field * f = _pbcM_sp_query(rm->msg->name, key);
		if (f == NULL) {
			// invalid key
			return NULL;
		}
		struct _message * m = f->type_name.m;

		if (m->def == NULL) {
			m->def = malloc(sizeof(struct pbc_rmessage));
			m->def->msg = m;
			m->def->index = NULL;
		}
		return m->def;
	} else {
		if (v->type->label == LABEL_REPEATED) {
			return _pbcA_index_p(v->v.array,index);
		} else {
			return &(v->v.message);
		}
	}
}

int 
pbc_rmessage_size(struct pbc_rmessage *m, const char *key) {
	struct value * v = _pbcM_sp_query(m->index,key);
	if (v == NULL) {
		return 0;
	}
	if (v->type->label == LABEL_REPEATED || v->type->label == LABEL_PACKED) {
		return pbc_array_size(v->v.array);
	} else {
		return 1;
	}
}