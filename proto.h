#ifndef PROTOBUFC_PROTO_H
#define PROTOBUFC_PROTO_H

#include "pbc.h"
#include "map.h"
#include "array.h"

#include <stdbool.h>
#include <stddef.h>

struct map_ip;
struct map_si;
struct map_sp;
struct _message;
struct _enum;

#define LABEL_OPTIONAL 0
#define LABEL_LABEL_REQUIRED 1
#define LABEL_REPEATED 2

struct _field {
	int id;
	const char *name;
	int type;
	int label;
	pbc_var default_v;
	union {
		const char * n;
		struct _message * m;
		struct _enum * e;
	} type_name;
};

struct _message {
	const char * key;
	struct map_ip * id;	// id -> _field
	struct map_sp * name;	// string -> _field
	struct pbc_rmessage * def;	// default message
};

struct _enum {
	const char * key;
	struct map_ip * id;
	struct map_si * name;
	pbc_var default_v;
};

struct pbc_env {
	struct map_sp * files;	// string -> void *
	struct map_sp * enums;	// string -> _enum
	struct map_sp * msgs;	// string -> _message
};

void proto_push_message(struct pbc_env * p, const char *name, struct _field *f , pbc_array queue);
struct _message * proto_init_message(struct pbc_env * p, const char *name);
struct _enum * proto_push_enum(struct pbc_env * p, const char *name, struct map_kv *table, int sz );
void proto_message_default(struct _message * m, const char * name, pbc_var defv);

struct _message * proto_get_message(struct pbc_env * p, const char *name);
struct _enum * proto_get_enum(struct pbc_env * p, const char *name);
void * pbc_array_index_p(pbc_array _array, int idx);

#endif
