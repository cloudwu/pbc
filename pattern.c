#include "pbc.h"
#include "alloc.h"
#include "context.h"
#include "varint.h"
#include "pattern.h"
#include "array.h"
#include "proto.h"
#include "map.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

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
			_pbcA_open(array);
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
	case PTYPE_ENUM:	// enum must be integer type in pattern mode
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
	_pbcA_push(_array , var);

	return 0;
}

void 
pbc_pattern_close_arrays(struct pbc_pattern *pat, void * data) {
	int i;
	for (i=0;i<pat->count;i++) {
		if (pat->f[i].ctype == CTYPE_ARRAY) {
			void *array = (char *)data + pat->f[i].offset;
			_pbcA_close(array);
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
				pbc_pattern_close_arrays(pat, output);
				_pbcC_close(_ctx);
				return -i-1;
			}
		}
	}
	_pbcC_close(_ctx);
	return 0;
}

/*
#define CTYPE_INT32 1
#define CTYPE_INT64 2
#define CTYPE_DOUBLE 3
#define CTYPE_FLOAT 4
#define CTYPE_POINTER 5
#define CTYPE_BOOL 6
#define CTYPE_INT8 7
#define CTYPE_INT16 8
#define CTYPE_ARRAY 9
#define CTYPE_VAR 10
*/

/* 
	format : key %type
	%f float
	%F double
	%d int32
	%D int64
	%b bool
	%h int16
	%c int8
	%s slice
	%a array
*/

static int
_ctype(const char * ctype) {
	if (ctype[0]!='%')
		return -1;
	switch (ctype[1]) {
	case 'f':
		return CTYPE_FLOAT;
	case 'F':
		return CTYPE_DOUBLE;
	case 'd':
		return CTYPE_INT32;
	case 'D':
		return CTYPE_INT64;
	case 'b':
		return CTYPE_BOOL;
	case 'h':
		return CTYPE_INT16;
	case 'c':
		return CTYPE_INT8;
	case 's':
		return CTYPE_VAR;
	case 'a':
		return CTYPE_ARRAY;
	default:
		return -1;
	}
}

static const char *
_copy_string(const char *format , char ** temp) {
	char * output = *temp;
	while (*format == ' ' || *format == '\t' || *format == '\n' || *format == '\r') {
		++format;
	}
	while (*format != '\0' &&
		*format != ' ' &&
		*format != '\t' &&
		*format != '\n' &&
		*format != '\r') {
		*output = *format;
		++output;
		++format;
	}
	*output = '\0';
	++output;
	*temp = output;

	return format;
}

static int
_scan_pattern(const char * format , char * temp) {
	int n = 0;
	for(;;) {
		format = _copy_string(format , &temp);
		if (format[0] == '\0')
			return 0;
		++n;
		format = _copy_string(format , &temp);
		if (format[0] == '\0')
			return n;
	} 
}

static int 
_comp_field(const void * a, const void * b) {
	const struct _pattern_field * fa = a;
	const struct _pattern_field * fb = b;

	return fa->id - fb->id;
}

struct pbc_pattern *
_pbcP_new(int n)
{
	size_t sz = sizeof(struct pbc_pattern) + (sizeof(struct _pattern_field)) * (n-1);
	struct pbc_pattern * ret = malloc(sz);
	memset(ret, 0 , sz);
	ret->count = n;
	return ret;
}

static int
_check_ctype(struct _field * field, struct _pattern_field *f) {
	if (field->label == LABEL_REPEATED) {
		return f->ctype != CTYPE_ARRAY;
	}
	if (field->type == PTYPE_STRING || field->type == PTYPE_MESSAGE) {
		return f->ctype != CTYPE_VAR;
	}
	if (field->type == PTYPE_FLOAT || field->type == PTYPE_DOUBLE) {
		return !(f->ctype == CTYPE_DOUBLE || f->ctype == CTYPE_FLOAT);
	}
	if (field->type == PTYPE_ENUM) {
		return !(f->ctype == CTYPE_INT8 || 
			f->ctype == CTYPE_INT8 || 
			f->ctype == CTYPE_INT16 ||
			f->ctype == CTYPE_INT32 ||
			f->ctype == CTYPE_INT64);
	}

	return f->ctype == CTYPE_VAR || f->ctype == CTYPE_ARRAY ||
		f->ctype == CTYPE_DOUBLE || f->ctype ==CTYPE_FLOAT;
}

struct pbc_pattern * 
pbc_pattern_new(struct pbc_env * env , const char * message, const char * format, ... ) {
	struct _message *m = proto_get_message(env, message);
	if (m==NULL) {
		return NULL;
	}
	int len = strlen(format);
	char temp[len+1];
	int n = _scan_pattern(format, temp);
	struct pbc_pattern * pat = _pbcP_new(n);
	int i;
	va_list ap;
	va_start(ap , format);

	const char *ptr = temp;

	for (i=0;i<n;i++) {
		struct _pattern_field * f = &(pat->f[i]);
		struct _field * field = _pbcM_sp_query(m->name, ptr);
		if (field == NULL)
			goto _error;
		f->id = field->id;
		f->ptype = field->type;
		*f->defv = *field->default_v;
		f->offset = va_arg(ap, int);

		ptr += strlen(ptr) + 1;

		f->ctype = _ctype(ptr);
		if (f->ctype < 0)
			goto _error;
		if (_check_ctype(field, f))
			goto _error;

		ptr += strlen(ptr) + 1;
	}

	va_end(ap);

	pat->count = n;

	qsort(pat->f , n , sizeof(struct _pattern_field), _comp_field);

	return pat;
_error:
	free(pat);
	return NULL;
}

void 
pbc_pattern_delete(struct pbc_pattern * pat) {
	free(pat);
}

