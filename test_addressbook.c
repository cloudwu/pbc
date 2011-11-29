#include "pbc.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

static void*
read_file (const char *filename , int * size) {
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;
	fseek(f,0,SEEK_END);
	long sz = ftell(f);
	fseek(f,0,SEEK_SET);
	void * buffer = malloc(sz);
	fread(buffer, 1 , sz , f);
	fclose(f);

	*size = sz;

	return buffer;
}

static void
dump(uint8_t *buffer, int sz) {
	int i , j;
	for (i=0;i<sz;i++) {
		printf("%02X ",buffer[i]);
		if (i % 16 == 15) {
			for (j = 0 ;j <16 ;j++) {
				char c = buffer[i/16 * 16+j];
				if (c>=32 && c<127) {
					printf("%c",c);
				} else {
					printf(".");
				}
			}
			printf("\n");
		}
	}
}

int
main()
{
	int sz = 0;
	void *buffer = read_file("addressbook.pb", &sz);
	if (buffer == NULL)
		return 1;
	struct pbc_env * env = pbc_new();
	pbc_register(env, buffer, sz);

	free(buffer);

	struct pbc_wmessage * msg = pbc_wmessage_new(env, "tutorial.Person");

	pbc_wmessage_string(msg, "name", "Alice", 0);
	pbc_wmessage_integer(msg, "id" , 12345, 0);
	pbc_wmessage_string(msg, "email", "alice@unkown", 0);

	struct pbc_wmessage * phone = pbc_wmessage_message(msg , "phone");
	pbc_wmessage_string(phone , "number", "87654321" , 0);

	phone = pbc_wmessage_message(msg , "phone");
	pbc_wmessage_string(phone , "number", "13901234567" , 0);
	pbc_wmessage_string(phone , "type" , "MOBILE" , 0);

	buffer = pbc_wmessage_buffer(msg, &sz);
	
	dump(buffer, sz);

	pbc_wmessage_delete(msg);
	pbc_delete(env);

	return 0;
}
