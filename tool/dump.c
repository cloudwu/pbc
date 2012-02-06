#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include "pbc.h"

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

//		printf("%s : %d\n", key, t);

static void dump_message(struct pbc_rmessage *m, int level);

static void
dump_value(struct pbc_rmessage *m, const char *key, int type, int idx, int level) {
	int i;
	for (i=0;i<level;i++) {
		printf("  ");
	}
	printf("%s",key);
	if (type & PBC_REPEATED) {
		printf("[%d]",idx);
		type -= PBC_REPEATED;
	}
	printf(" : ");

	uint32_t low;
	uint32_t hi;
	double real;
	const char *str;

	switch(type) {
	case PBC_INT:
		low = pbc_rmessage_integer(m, key, i, NULL);
		printf("%d", (int) low);
		break;
	case PBC_REAL:
		real = pbc_rmessage_real(m, key , i);
		printf("%lf", real);
		break;
	case PBC_BOOL:
		low = pbc_rmessage_integer(m, key, i, NULL);
		printf("%s", low ? "true" : "false");
		break;
	case PBC_ENUM:
		str = pbc_rmessage_string(m, key , i , NULL);
		printf("[%s]", str);
		break;
	case PBC_STRING:
		str = pbc_rmessage_string(m, key , i , NULL);
		printf("'%s'", str);
		break;
	case PBC_MESSAGE:
		printf("\n");
		dump_message(pbc_rmessage_message(m, key, i),level+1);
		return;
	case PBC_FIXED64:
		low = pbc_rmessage_integer(m, key, i, &hi);
		printf("0x%8x%8x",hi,low);
		break;
	case PBC_FIXED32:
		low = pbc_rmessage_integer(m, key, i, NULL);
		printf("0x%x",low);
		break;
	default:
		printf("unkown");
		break;
	}

	printf("\n");
}

static void
dump_message(struct pbc_rmessage *m, int level) {
	int t = 0;
	const char *key = NULL;
	for (;;) {
		t = pbc_rmessage_next(m, &key);
		if (key == NULL)
			break;
		if (t & PBC_REPEATED) {
			int n = pbc_rmessage_size(m, key);
			int i;
			for (i=0;i<n;i++) {
				dump_value(m, key , t , i , level);
			}
		} else {
			dump_value(m, key , t , 0 , level);
		}
	}
}

static void
dump(const char *proto, const char * message, const char *datafile) {
	struct pbc_env * env = pbc_new();
	struct pbc_slice pb,data;
	read_file(proto, &pb);
	int r = pbc_register(env, &pb);
	if (r!=0) {
		fprintf(stderr, "Can't register %s\n", proto);
		exit(1);
	}
	read_file(datafile , &data);
	struct pbc_rmessage * m = pbc_rmessage_new(env , message , &data);
	if (m == NULL) {
		fprintf(stderr, "Decode message %s fail\n",message);
		exit(1);
	}
	dump_message(m,0);
}

static void
usage(const char *argv0) {
	printf("  -h help.\n"
		"  -p <filename.pb> protobuf file\n"
		"  -m <messagename>\n"
		"  -d <datafile>\n");
}

int
main(int argc , char * argv[])
{
	int ch;
	const char * proto = NULL;
	const char * message = NULL;
	const char * data = NULL;
	while ((ch = getopt(argc, argv, "hp:m:d:")) != -1) {
		switch(ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'p':
			proto = optarg;
			break;
		case 'm':
			message = optarg;
			break;
		case 'd':
			data = optarg;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}
	if (proto == NULL || message == NULL || data == NULL) {
		usage(argv[0]);
		return 1;
	}

	dump(proto , message , data);

	return 0;
}