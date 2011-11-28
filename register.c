#include "pbc.h"
#include "proto.h"
#include "alloc.h"
#include "map.h"
#include "bootstrap.h"
#include "context.h"
#include "stringpool.h"

#include <string.h>
#include <stdlib.h>

static const char *
_concat_name(struct _stringpool *p , const char *prefix ,  int prefix_sz , const char *name , int name_sz) {
	char temp[name_sz + prefix_sz + 2];
	memcpy(temp,prefix,prefix_sz);
	temp[prefix_sz] = '.';
	memcpy(temp+prefix_sz+1,name,name_sz);
	temp[name_sz + prefix_sz + 1] = '\0';

	return _pbcS_build(p , temp, name_sz + prefix_sz + 1);
}

static void
_register_enum(struct pbc_env *p, struct _stringpool *pool, struct pbc_rmessage * enum_type, const char *prefix, int prefix_sz) {
	int field_count = pbc_rmessage_size(enum_type, "value");
	struct map_kv *table = malloc(field_count * sizeof(struct map_kv));
	int i;
	for (i=0;i<field_count;i++) {
		struct pbc_rmessage * value = pbc_rmessage_message(enum_type, "value", i);
		int enum_name_sz;
		const char *enum_name = pbc_rmessage_string(value , "name" , 0 , &enum_name_sz);
		table[i].pointer = (void *)_pbcS_build(pool, enum_name , enum_name_sz);
		table[i].id = pbc_rmessage_integer(value , "number", 0 , 0);
	}
	int name_sz;
	const char * name = pbc_rmessage_string(enum_type, "name", 0 , &name_sz);
	const char *temp = _concat_name(pool, prefix , prefix_sz , name , name_sz);

	proto_push_enum(p,temp,table,field_count);
	free(table);
}

static void
_set_default(struct _stringpool *pool, struct _field *f , int ptype, const char *value, int sz) {
	if (value == NULL || sz == 0) {
		if (f->type == PTYPE_STRING) {
			f->default_v->s.str = "";
			f->default_v->s.len = 0;
		} else {
			f->default_v->integer.low = 0;
			f->default_v->integer.hi = 0;
		}
		return;
	}

	switch (f->type) {
	case PTYPE_DOUBLE:
	case PTYPE_FLOAT:
		f->default_v->real = strtod(value,NULL);
		break;
	case PTYPE_STRING:
		f->default_v->s.str = _pbcS_build(pool, value , sz);
		f->default_v->s.len = sz;
		break;
	case PTYPE_ENUM:
		f->default_v->s.str = value;
		f->default_v->s.len = sz;
		break;
	case PTYPE_BOOL:
		if (strcmp(value,"true") == 0) {
			f->default_v->integer.low = 1;
		} else {
			f->default_v->integer.low = 0;
		}
		f->default_v->integer.hi = 0;
		break;
	case PTYPE_INT64:
	case PTYPE_UINT64:
	case PTYPE_SFIXED64:
	case PTYPE_SINT64:
		// todo string to int64
	case PTYPE_INT32:
	case PTYPE_FIXED32:
	case PTYPE_UINT32:
	case PTYPE_SFIXED32:
	case PTYPE_SINT32:
		f->default_v->integer.low = atol(value);
		f->default_v->integer.hi = 0;
		break;
	default:
		f->default_v->integer.low = 0;
		f->default_v->integer.hi = 0;
		break;
	}
}

static void
_register_message(struct pbc_env *p, struct _stringpool *pool, struct pbc_rmessage * message_type, const char *prefix, int prefix_sz, pbc_array queue) {
	int name_sz;
	const char * name = pbc_rmessage_string(message_type, "name", 0 , &name_sz);
	const char *temp = _concat_name(pool, prefix , prefix_sz , name , name_sz);

	int field_count = pbc_rmessage_size(message_type, "field");
	int i;
	for (i=0;i<field_count;i++) {
		struct pbc_rmessage * field = pbc_rmessage_message(message_type, "field" , i);
		struct _field f;
		f.id = pbc_rmessage_integer(field, "number", 0 , 0);
		int field_name_sz;
		const char * field_name = pbc_rmessage_string(field, "name", 0 , &field_name_sz);
		f.name = _pbcS_build(pool,field_name,field_name_sz);
		f.type = pbc_rmessage_integer(field, "type", 0 , 0);	// enum
		f.label = pbc_rmessage_integer(field, "label", 0, 0) - 1; // LABEL_OPTIONAL = 0
		f.type_name.n = pbc_rmessage_string(field, "type_name", 0 , NULL) +1;	// abandon prefix '.' 
		int vsz;
		const char * default_value = pbc_rmessage_string(field, "default_value", 0 , &vsz);
		_set_default(pool , &f , f.type, default_value , vsz);
		proto_push_message(p, temp , &f , queue);
	}
	if (field_count > 0) {
		proto_init_message(p, temp);
	}

	// todo: extension

	// nested enum

	int enum_count = pbc_rmessage_size(message_type, "enum_type");

	for (i=0;i<enum_count;i++) {
		struct pbc_rmessage * enum_type = pbc_rmessage_message(message_type, "enum_type", i);
		_register_enum(p, pool, enum_type, temp, name_sz + prefix_sz + 1);
	}
	
	// nested type
	int message_count = pbc_rmessage_size(message_type, "nested_type");
	for (i=0;i<message_count;i++) {
		struct pbc_rmessage * nested_type = pbc_rmessage_message(message_type, "nested_type", i);
		_register_message(p, pool, nested_type, temp, name_sz + prefix_sz + 1, queue);
	}
}

static void
_register(struct pbc_env *p, struct pbc_rmessage * file, struct _stringpool *pool) {
	int package_sz;
	const char *package = pbc_rmessage_string(file, "package", 0, &package_sz);

	pbc_array queue;
	pbc_array_open(queue);

	int enum_count = pbc_rmessage_size(file, "enum_type");
	int i;

	for (i=0;i<enum_count;i++) {
		struct pbc_rmessage * enum_type = pbc_rmessage_message(file, "enum_type", i);
		_register_enum(p,  pool , enum_type, package, package_sz);
	}

	int message_count = pbc_rmessage_size(file, "message_type");
	for (i=0;i<message_count;i++) {
		struct pbc_rmessage * message_type = pbc_rmessage_message(file, "message_type", i);
		_register_message(p, pool, message_type, package, package_sz, queue);
	}

	_pbcB_register_fields(p, queue);

	pbc_array_close(queue);
}

static const char *
_check_file_name(struct pbc_env * p , struct pbc_rmessage * file, void *data) {
	const char * filename = pbc_rmessage_string(file, "name", 0, NULL);
	if (_pbcM_sp_query(p->files, filename)) {
		return filename;
	}
	int sz = pbc_rmessage_size(file, "dependency"); 
	int i;
	for (i=0;i<sz;i++) {
		const char *dname = pbc_rmessage_string(file,"dependency",i,NULL);
		if (_pbcM_sp_query(p->files, dname) == NULL) {
			return dname;
		}
	}

	_pbcM_sp_insert(p->files , filename, data);
	return NULL;
}

int
pbc_register(struct pbc_env * p, void *buffer, int size) {
	struct pbc_rmessage * message = pbc_rmessage_new(p, "google.protobuf.FileDescriptorSet", buffer,size);
	if (message == NULL) {
		return 1;
	}

	struct pbc_rmessage * file = pbc_rmessage_message(message, "file", 0);
	if (file == NULL) {
		goto _error;
	}
	struct _stringpool *pool = _pbcS_new();
	if (_check_file_name(p, file , pool)) {
		_pbcS_delete(pool);
		goto _error;
	}

	_register(p,file,pool);

	pbc_rmessage_delete(message);
	return 0;
_error:
	pbc_rmessage_delete(message);
	return 1;
}
