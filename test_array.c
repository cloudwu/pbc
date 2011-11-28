#include "pbc.h"
#include "alloc.h"

#include <stdio.h>

int
main()
{
	pbc_array array;
	pbc_var v;

	pbc_array_open(array);

	int i ;

	for (i=0;i<100;i++) {
		v->real = (double)i;
		printf("push %d\n",i);
		pbc_array_push(array, v);
	}

	int s = pbc_array_size(array);

	for (i=0;i<s;i++) {
		pbc_array_index(array, i , v);
		printf("%lf\n",v->real);
	}

	pbc_array_close(array);

	return 0;
}
