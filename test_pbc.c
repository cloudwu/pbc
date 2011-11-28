#include "pbc.h"
#include "alloc.h"

#include <stdlib.h>
#include <stdio.h>

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

void
test_des(struct pbc_env * env)
{
	int sz = 0;
	void *buffer = read_file("descriptor.pb", &sz);

	struct pbc_rmessage * msg = pbc_rmessage_new(env, "google.protobuf.FileDescriptorSet", buffer, sz);

	struct pbc_rmessage * file = pbc_rmessage_message(msg,"file",0);

	printf("name = %s\n",pbc_rmessage_string(file, "name", 0 , NULL));
	printf("package = %s\n",pbc_rmessage_string(file, "package", 0 , NULL));

	sz = pbc_rmessage_size(file, "dependency");
	printf("dependency[%d] =\n" , sz);
	int i;
	for (i=0;i<sz;i++) {
		printf("\t%s\n",pbc_rmessage_string(file, "dependency", i , NULL));
	}
	sz = pbc_rmessage_size(file, "message_type");
	printf("message_type[%d] = \n",sz);
	for (i=0;i<sz;i++) {
		struct pbc_rmessage * message_type = pbc_rmessage_message(file,"message_type",i);
		printf("\tname = %s\n",pbc_rmessage_string(message_type,"name",0,NULL));
	}


	pbc_rmessage_delete(msg);
	free(buffer);

	struct pbc_wmessage * wmsg = pbc_wmessage_new(env , "google.protobuf.EnumDescriptorProto");
	pbc_wmessage_string(wmsg,"name","test",0);
	struct pbc_wmessage * value = pbc_wmessage_message(wmsg,"value");
	pbc_wmessage_string(value,"name","Hello",0);
	pbc_wmessage_integer(value,"value",1,0);

	value = pbc_wmessage_message(wmsg,"value");
	pbc_wmessage_string(value,"name","World",0);
	pbc_wmessage_integer(value,"number",2,0);

	pbc_wmessage_delete(wmsg);
}

int
main()
{
	struct pbc_env * env = pbc_new();

//	memory();

	test_des(env);

//	memory();

	pbc_delete(env);

	memory();

	return 0;
}
