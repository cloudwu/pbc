#ifndef PROTOBUF_C_CONTEXT_H
#define PROTOBUF_C_CONTEXT_H

#include <stdint.h>

#include "array.h"

#define PBC_CONTEXT_CAP 256

// wiretype

#define WT_VARINT 0
#define WT_BIT64 1
#define WT_LEND 2
#define WT_BIT32 5

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

#define PTYPE_DOUBLE   1
#define PTYPE_FLOAT    2
#define PTYPE_INT64    3   // Not ZigZag encoded.  Negative numbers take 10 bytes.  Use TYPE_SINT64 if negative values are likely.
#define PTYPE_UINT64   4
#define PTYPE_INT32    5   // Not ZigZag encoded.  Negative numbers take 10 bytes.  Use TYPE_SINT32 if negative values are likely.
#define PTYPE_FIXED64  6
#define PTYPE_FIXED32  7
#define PTYPE_BOOL     8
#define PTYPE_STRING   9
#define PTYPE_GROUP    10  // Tag-delimited aggregate.
#define PTYPE_MESSAGE  11  // Length-delimited aggregate.
#define PTYPE_BYTES    12
#define PTYPE_UINT32   13
#define PTYPE_ENUM     14
#define PTYPE_SFIXED32 15
#define PTYPE_SFIXED64 16
#define PTYPE_SINT32   17  // Uses ZigZag encoding.
#define PTYPE_SINT64   18  // Uses ZigZag encoding.

struct slice {
	int start;
	int end;
};

struct atom {
	int id;
	union {
		struct slice s;
		struct longlong i;
	} v;
};

struct context {
	char * buffer;
	int size;
	int number;
	struct atom * a;
};

typedef struct _pbc_ctx { char _data[PBC_CONTEXT_CAP]; } pbc_ctx[1];

int _pbcC_open(pbc_ctx , void *buffer, int size);	// <=0 failed
void _pbcC_close(pbc_ctx);

static inline double
read_double(struct atom * a) {
	union {
		uint64_t i;
		double d;
	} u;
	u.i = (uint64_t) a->v.i.low | (uint64_t) a->v.i.hi << 32;
	return u.d;
}

static inline float
read_float(struct atom * a) {
	union {
		uint32_t i;
		float f;
	} u;
	u.i = a->v.i.low;
	return u.f;
}

#endif
