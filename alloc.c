#include <stdlib.h>
#include <stdio.h>

static int _g = 0;

void * _pbcM_malloc(size_t sz) {
	++ _g;
	return malloc(sz);
}

void _pbcM_free(void *p) {
	if (p) {
		-- _g;
		free(p);
	}
}

void* _pbcM_realloc(void *p, size_t sz) {
	return realloc(p,sz);
}

void _pbcM_memory() {
	printf("%d\n",_g);	
}
