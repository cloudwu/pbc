#include "pbc.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

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

	printf("\n");
}

struct person_phone {
	struct pbc_slice number;
	int32_t type;
};

struct person {
	struct pbc_slice name;
	int32_t id;
	struct pbc_slice email;
	pbc_array phone;
	pbc_array test;
};

static void
test_pattern(struct pbc_env *env, void *buffer, int size) {
	struct pbc_pattern * pat = pbc_pattern_new(env, "tutorial.Person" , 
		"name %s id %d email %s phone %a test %a",
		offsetof(struct person, name) , 
		offsetof(struct person, id) ,
		offsetof(struct person, email) ,
		offsetof(struct person, phone) ,
		offsetof(struct person, test));

	// enum must be integer
	struct pbc_pattern * pat_phone = pbc_pattern_new(env, "tutorial.Person.PhoneNumber",
		"number %s type %d",
		offsetof(struct person_phone, number),
		offsetof(struct person_phone, type));

	struct person p;
	int r = pbc_pattern_unpack(pat, buffer, size, &p);
	if (r>=0) {
		printf("name = %s\n",(const char *)p.name.buffer);
		printf("id = %d\n",p.id);
		printf("email = %s\n",(const char *)p.email.buffer);
		int n = pbc_array_size(p.phone);
		int i;
		for (i=0;i<n;i++) {
			struct pbc_slice * bytes = pbc_array_bytes(p.phone, i);
			struct person_phone pp;
			pbc_pattern_unpack(pat_phone , bytes->buffer, bytes->len , &pp);
			printf("\tnumber = %s\n" , (const char*)pp.number.buffer);
			printf("\ttype = %d\n" , pp.type);
		}

		n = pbc_array_size(p.test);
		for (i=0;i<n;i++) {
			printf("test[%d] = %d\n",i, pbc_array_integer(p.test, i , NULL));
		}

		pbc_pattern_close_arrays(pat,&p);
	}

	pbc_pattern_delete(pat);
	pbc_pattern_delete(pat_phone);
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

	pbc_wmessage_string(msg, "name", "Alice", -1);
	pbc_wmessage_integer(msg, "id" , 12345, 0);
	pbc_wmessage_string(msg, "email", "alice@unkown", -1);

	struct pbc_wmessage * phone = pbc_wmessage_message(msg , "phone");
	pbc_wmessage_string(phone , "number", "87654321" , -1);

	phone = pbc_wmessage_message(msg , "phone");
	pbc_wmessage_string(phone , "number", "13901234567" , -1);
	pbc_wmessage_string(phone , "type" , "MOBILE" , 0);

	pbc_wmessage_integer(msg, "test", 1,0);
	pbc_wmessage_integer(msg, "test", 2,0);
	pbc_wmessage_integer(msg, "test", 3,0);

	buffer = pbc_wmessage_buffer(msg, &sz);
	
	dump(buffer, sz);

	test_pattern(env, buffer, sz);

	pbc_wmessage_delete(msg);
	pbc_delete(env);

	return 0;
}
