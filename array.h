#ifndef PROTOBUF_C_ARRAY_H
#define PROTOBUF_C_ARRAY_H

#include "varint.h"

typedef union _pbc_var {
	struct longlong integer;
	double real;
	struct {
		const char * str;
		int len;
	} s;
	struct {
		int id;
		const char * name;
	} e;
	struct {
		void * buffer;
		int len;
	} m;
	void * p[2];
} pbc_var[1];

void _pbcA_open(pbc_array);
void _pbcA_close(pbc_array);
int _pbcA_size(pbc_array);

void _pbcA_push(pbc_array, pbc_var var);
void _pbcA_index(pbc_array , int idx, pbc_var var);
void * _pbcA_index_p(pbc_array _array, int idx);

#endif
