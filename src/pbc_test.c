#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "pbc.h"
#include "proto.h"

static void
read_file (const char *filename , struct pbc_slice *slice) {
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr, "Can't open file %s\n", filename);
		exit(1);
	}
	fseek(f,0,SEEK_END);
	slice->len = ftell(f);
	fseek(f,0,SEEK_SET);
	slice->buffer = malloc(slice->len);
	fread(slice->buffer, 1 , slice->len , f);
	fclose(f);
}

static void
dump_message(struct _message *m, int level) {
	int t = 0;
	const char *key = NULL;
	for (;;) {
		 struct value * v = (struct value *)_pbcM_sp_next(m->name, &key);
		 if(key) {
		 	printf("key=%s\n", key);
		 }
	}
}

static void
dump(const char *proto /*, const char * message, struct pbc_slice *data*/) {
	struct pbc_env * env = pbc_new();
	struct pbc_slice pb;
	read_file(proto, &pb);
	int r = pbc_register(env, &pb);
	if (r!=0) {
		fprintf(stderr, "Can't register %s\n", proto);
		exit(1);
	}
	//struct pbc_wmessage * m = pbc_wmessage_new(env, "Person");
	struct _message * msg = _pbcP_get_message(env, "Person");/*
	struct pbc_rmessage * m = pbc_rmessage_new(env , message , data);
	if (m == NULL) {
		fprintf(stderr, "Decode message %s fail\n",message);
		exit(1);
	}
	*/
	dump_message(msg, 0);
}


void
test_des(struct pbc_env * env , const char * pb)
{
	struct pbc_slice slice;
	read_file(pb, &slice);

	struct pbc_rmessage * msg = pbc_rmessage_new(env, "google.protobuf.FileDescriptorSet", &slice);

	struct pbc_rmessage * file = pbc_rmessage_message(msg,"file",0);

	printf("name = %s\n",pbc_rmessage_string(file, "name", 0 , NULL));
	printf("package = %s\n",pbc_rmessage_string(file, "package", 0 , NULL));

	int sz = pbc_rmessage_size(file, "dependency");
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

	int ret = pbc_register(env, &slice);

	printf("Register %d\n",ret);

	free(slice.buffer);
}


int
main(int argc , char * argv[])
{
	struct pbc_env * env = pbc_new();
	
	//read_file(argv[1], &data);
	test_des(env, argv[1]);
	//dump(argv[1]);
	return 0;
}