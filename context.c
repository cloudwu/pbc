#include "pbc.h"
#include "alloc.h"
#include "varint.h"
#include "context.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define INNER_ATOM ((PBC_CONTEXT_CAP - sizeof(struct context)) / sizeof(struct atom))

#define MIN(a,b) ((a) < (b) ? (a) : (b))

static char * 
wiretype_decode(char *buffer, int cap , struct atom *a , int start)
{
	uint8_t temp[10];
	memcpy(temp, buffer , MIN(cap,10));
	struct longlong r;
	int len = varint_decode(temp, &r);
	if (len > cap || r.hi !=0)
		return NULL;
	int wiretype = r.low & 7;
	a->id = r.low >> 3;
	buffer += len;
	start += len;
	cap -=len;
	
	switch (wiretype) {
	case WT_VARINT :
		memcpy(temp, buffer , MIN(cap,10));
		len = varint_decode(temp, &a->v.i);
		if (cap < len)
			return NULL;
		return buffer+len;
	case WT_BIT64 :
		if (cap < 8)
			return NULL;
		a->v.i.low = buffer[0] |
			buffer[1] << 8 |
			buffer[2] << 16 |
			buffer[3] << 24;
		a->v.i.hi = buffer[4] |
			buffer[5] << 8 |
			buffer[6] << 16 |
			buffer[7] << 24;
		return buffer + 8;
	case WT_LEND :
		memcpy(temp, buffer , MIN(cap,10));
		len = varint_decode(temp, &r);
		if (cap < len + r.low || r.hi !=0)
			return NULL;
		a->v.s.start = start + len;
		a->v.s.end = start + len + r.low;
		return buffer + len + r.low;
		break;
	case WT_BIT32 :
		if (cap < 4)
			return NULL;
		a->v.i.low = buffer[0] |
			buffer[1] << 8 |
			buffer[2] << 16 |
			buffer[3] << 24;
		a->v.i.hi = 0;
		return buffer + 4;
		break;
	default:
		return NULL;
	}
}

int 
_pbcC_open(pbc_ctx _ctx , void *buffer, int size) {
	struct context * ctx = (struct context *)_ctx;
	ctx->buffer = buffer;
	ctx->size = size;

	if (buffer == NULL || size == 0) {
		ctx->number = 0;
		ctx->a = NULL;
		return 0;
	}

	struct atom * a = (struct atom *)(ctx + 1);

	int i;
	int start = 0;

	for (i=0;i<INNER_ATOM;i++) {
		if (size == 0)
			break;
		char * next = wiretype_decode(buffer, size , &a[i] , start);
		if (next == NULL)
			return -i;
		start += next - (char *)buffer;
		size -= next - (char *)buffer;
		buffer = next;
	}

	if (size == 0) {
		ctx->a = a;
	} else {
		int cap = 64;
		ctx->a = malloc(cap * sizeof(struct atom));
		while (size != 0) {
			if (i >= cap) {
				cap = cap + 64;
				ctx->a = realloc(ctx->a, cap * sizeof(struct atom));
				continue;
			}
			char * next = wiretype_decode(buffer, size , &ctx->a[i] , start);
			if (next == NULL) {
				return -i;
			}
			start += next - (char *)buffer;
			size -= next - (char *)buffer;
			buffer = next;
			++i;
		}
		memcpy(ctx->a, a , sizeof(struct atom) * INNER_ATOM);
	}
	ctx->number = i;

	return i;
}


void 
_pbcC_close(pbc_ctx _ctx) {
	struct context * ctx = (struct context *)_ctx;
	if (ctx->a != NULL && (struct atom *)(ctx+1) != ctx->a) {
		free(ctx->a);
		ctx->a = NULL;
	}
}
