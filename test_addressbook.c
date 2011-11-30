#include "pbc.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

static void
read_file (const char *filename , struct pbc_slice *slice) {
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		slice->buffer = NULL;
		slice->len = 0;
		return;
	}
	fseek(f,0,SEEK_END);
	slice->len = ftell(f);
	fseek(f,0,SEEK_SET);
	slice->buffer = malloc(slice->len);
	fread(slice->buffer, 1 , slice->len , f);
	fclose(f);
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
test_pattern(struct pbc_env *env, struct pbc_slice * slice) {
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
	int r = pbc_pattern_unpack(pat, slice, &p);
	if (r>=0) {
		printf("name = %s\n",(const char *)p.name.buffer);
		printf("id = %d\n",p.id);
		printf("email = %s\n",(const char *)p.email.buffer);
		int n = pbc_array_size(p.phone);
		int i;
		for (i=0;i<n;i++) {
			struct pbc_slice * bytes = pbc_array_bytes(p.phone, i);
			struct person_phone pp;
			pbc_pattern_unpack(pat_phone , bytes , &pp);
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

static void
test_rmessage(struct pbc_env *env, struct pbc_slice *slice) {
	struct pbc_rmessage * m = pbc_rmessage_new(env, "tutorial.Person", slice);
	printf("name = %s\n", pbc_rmessage_string(m , "name" , 0 , NULL));
	printf("id = %d\n", pbc_rmessage_integer(m , "id" , 0 , NULL));
	printf("email = %s\n", pbc_rmessage_string(m , "email" , 0 , NULL));

	int phone_n = pbc_rmessage_size(m, "phone");
	int i;

	for (i=0;i<phone_n;i++) {
		struct pbc_rmessage * p = pbc_rmessage_message(m , "phone", i);
		printf("\tnumber[%d] = %s\n",i,pbc_rmessage_string(p , "number", i ,NULL));
		printf("\ttype[%d] = %s\n",i,pbc_rmessage_string(p, "type", i, NULL));
	}

	int n = pbc_rmessage_size(m , "test");

	for (i=0;i<n;i++) {
		printf("test[%d] = %d\n",i, pbc_rmessage_integer(m , "test" , i , NULL));
	}
	pbc_rmessage_delete(m);
}

static struct pbc_wmessage *
test_wmessage(struct pbc_env * env)
{
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

	return msg;
}

int
main()
{
	struct pbc_slice slice;
	read_file("addressbook.pb", &slice);
	if (slice.buffer == NULL)
		return 1;
	struct pbc_env * env = pbc_new();
	pbc_register(env, &slice);

	free(slice.buffer);

	struct pbc_wmessage *msg = test_wmessage(env);

	pbc_wmessage_buffer(msg, &slice);

	dump(slice.buffer, slice.len);

	test_rmessage(env, &slice);

	test_pattern(env, &slice);

	pbc_wmessage_delete(msg);
	pbc_delete(env);

	return 0;
}
