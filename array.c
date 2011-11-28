#include "pbc.h"
#include "alloc.h"

#include <stdlib.h>
#include <string.h>

struct array {
	int number;
	union _pbc_var * a;
};

#define INNER_FIELD ((PBC_ARRAY_CAP - sizeof(struct array)) / sizeof(pbc_var))

void 
pbc_array_open(pbc_array _array) {
	struct array * a = (struct array *)_array;
	a->number = 0;
	a->a = (union _pbc_var *)(a+1);
}

void 
pbc_array_close(pbc_array _array) {
	struct array * a = (struct array *)_array;
	if (a->a != NULL && (union _pbc_var *)(a+1) != a->a) {
		free(a->a);
		a->a = NULL;
	}
}

void 
pbc_array_push(pbc_array _array, pbc_var var) {
	struct array * a = (struct array *)_array;
	if (a->number >= INNER_FIELD) {
		if (a->number == INNER_FIELD) {
			int cap = 1;
			while (cap <= a->number + 1) 
				cap *= 2;
			union _pbc_var * outer = malloc(cap * sizeof(union _pbc_var));
			memcpy(outer , a->a , INNER_FIELD * sizeof(pbc_var));
			a->a = outer;
		} else {
			int size=a->number;
			if (((size + 1) ^ size) > size) {
			   a->a=realloc(a->a,sizeof(union _pbc_var) * (size+1) * 2);
			}
		}
	}
	a->a[a->number] = *var;
	++ a->number;
}

void 
pbc_array_index(pbc_array _array, int idx, pbc_var var)
{
	struct array * a = (struct array *)_array;
	var[0] = a->a[idx];
}

void *
pbc_array_index_p(pbc_array _array, int idx)
{
	struct array * a = (struct array *)_array;
	return &(a->a[idx]);
}

int 
pbc_array_size(pbc_array _array) {
	struct array * a = (struct array *)_array;
	return a->number;
}
