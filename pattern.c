#include "pbc.h"
#include "alloc.h"
#include "context.h"
#include "varint.h"
#include "pattern.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

static void
set_default_v(void * output, int ctype, pbc_var defv) {
	switch (ctype) {
	case CTYPE_INT32:
		*(uint32_t *)output = defv->integer.low;
		break;
	case CTYPE_INT64:
		*(uint64_t *)output = (uint64_t)defv->integer.low | (uint64_t)defv->integer.hi << 32;
		break;
	case CTYPE_DOUBLE:
		*(double *)output = defv->real;
		break;
	case CTYPE_FLOAT:
		*(float *)output = (float)defv->real;
		break;
	case CTYPE_BOOL:
		*(bool *)output = (defv->integer.low != 0);
		break;
	case CTYPE_INT8:
		*(uint8_t *)output = (uint8_t)defv->integer.low;
		break;
	case CTYPE_INT16:
		*(uint16_t *)output = (uint16_t)defv->integer.low;
		break;
	case CTYPE_VAR:
		*(union _pbc_var *)output = *defv;
		break;
	}
}

static void
set_default(struct pbc_pattern *pat, uint8_t * output) {
	int i;
	for (i=0;i<pat->count;i++) {
		if (pat->f[i].ctype == CTYPE_ARRAY) {
			struct _pbc_array *array = (struct _pbc_array *)(output + pat->f[i].offset);
			pbc_array_open(array);
			continue;
		} else if (pat->f[i].ptype == PTYPE_MESSAGE) {
			struct _pbc_ctx *ctx = (struct _pbc_ctx *)(output + pat->f[i].offset);
			_pbcC_open(ctx , NULL , 0);			
			continue;
		} else if (pat->f[i].ptype == PTYPE_ENUM) {
			pbc_var defv;
			defv->integer.low = pat->f[i].defv->e.id;
			defv->integer.hi = 0;
			set_default_v(output + pat->f[i].offset, pat->f[i].ctype, defv);
			continue;
		}
		set_default_v(output + pat->f[i].offset, pat->f[i].ctype, pat->f[i].defv);
	}
}

// pattern unpack

static struct _pattern_field *
bsearch_pattern(struct pbc_pattern *pat, int id)
{
	int begin = 0;
	int end = pat->count;
	while (begin < end) {
		int mid = (begin + end)/2;
		struct _pattern_field * f = &pat->f[mid];
		if (id == f->id) {
			return f;
		}
		if (id < f->id) {
			end = mid;
		} else {
			begin = mid + 1;
		}
	}
	return NULL;
}

static inline int
write_real(int ctype, double v, void *out) {
	switch(ctype) {
	case CTYPE_DOUBLE:
		*(double *)out = v;
		return 0;
	case CTYPE_FLOAT:
		*(float *)out = (float)v;
		return 0;
	case CTYPE_VAR:
		((union _pbc_var *)out)->real = v;
		return 0;
	}
	return -1;
}

static inline int
write_longlong(int ctype, struct longlong *i, void *out) {
	switch(ctype) {
	case CTYPE_INT32:
		*(uint32_t *)out = i->low;
		return 0;
	case CTYPE_INT64:
		*(uint64_t *)out = (uint64_t)i->low | (uint64_t)i->hi << 32;
		return 0;
	case CTYPE_BOOL:
		*(bool *)out = (i->low !=0) ;
		return 0;
	case CTYPE_INT8:
		*(uint8_t *)out = (uint8_t)i->low;
		return 0;
	case CTYPE_INT16:
		*(uint8_t *)out = (uint16_t)i->low;
		return 0;
	case CTYPE_VAR:
		((union _pbc_var *)out)->integer = *i;
		return 0;
	}
	return -1;
}

static inline int
write_integer(int ctype, struct atom *a, void *out) {
	return write_longlong(ctype, &(a->v.i), out);
}

static int unpack_array(int ptype, char *buffer, struct atom *, pbc_array _array);

static int
unpack_field(int ctype, int ptype, char * buffer, struct atom * a, void *out) {
	if (ctype == CTYPE_ARRAY) {
		return unpack_array(ptype, buffer, a , out);
	}
	switch(ptype) {
	case PTYPE_DOUBLE:
		return write_real(ctype, read_double(a), out);
	case PTYPE_FLOAT:
		return write_real(ctype, read_float(a), out);
	case PTYPE_INT64:
	case PTYPE_UINT64:
	case PTYPE_INT32:
	case PTYPE_UINT32:
	case PTYPE_FIXED32:
	case PTYPE_FIXED64:
	case PTYPE_SFIXED32:
	case PTYPE_SFIXED64:
	case PTYPE_ENUM:	// todo
	case PTYPE_BOOL:
		return write_integer(ctype, a , out);
	case PTYPE_SINT32: {
		struct longlong temp = a->v.i;
		varint_dezigzag32(&temp);
		return write_longlong(ctype, &temp , out);
	}
	case PTYPE_SINT64: {
		struct longlong temp = a->v.i;
		varint_dezigzag64(&temp);
		return write_longlong(ctype, &temp , out);
	}
	case PTYPE_MESSAGE: 
		((union _pbc_var *)out)->m.buffer = buffer + a->v.s.start;
		((union _pbc_var *)out)->m.len = a->v.s.end - a->v.s.start;
		return 0;
	case PTYPE_STRING:
		((union _pbc_var *)out)->s.str = buffer + a->v.s.start;
		((union _pbc_var *)out)->s.len = a->v.s.end - a->v.s.start;
		return 0;
	}
	return -1;
}

static int 
unpack_array(int ptype, char *buffer, struct atom * a, pbc_array _array) {
	pbc_var var;
	int r = unpack_field(CTYPE_VAR, ptype, buffer, a , var);
	if (r !=0 )
		return r;
	pbc_array_push(_array , var);

	return 0;
}

void 
pbc_pattern_close(struct pbc_pattern *pat, void * data) {
	int i;
	for (i=0;i<pat->count;i++) {
		if (pat->f[i].ctype == CTYPE_ARRAY) {
			void *array = (char *)data + pat->f[i].offset;
			pbc_array_close(array);
		}
	}
}

int 
pbc_pattern_unpack(struct pbc_pattern *pat, void *buffer, int sz, void * output) {
	pbc_ctx _ctx;
	int r = _pbcC_open(_ctx, buffer, sz);
	if (r <= 0) {
		_pbcC_close(_ctx);
		return r+1;
	}
	set_default(pat, output);

	struct context * ctx = (struct context *)_ctx;

	int i;

	for (i=0;i<ctx->number;i++) {
		struct _pattern_field * f = bsearch_pattern(pat, ctx->a[i].id);
		if (f) {
			char * out = (char *)output + f->offset;
			if (unpack_field(f->ctype , f->ptype , ctx->buffer , &ctx->a[i], out) != 0) {
				pbc_pattern_close(pat, output);
				_pbcC_close(_ctx);
				return -i-1;
			}
		}
	}
	_pbcC_close(_ctx);
	return 0;
}


