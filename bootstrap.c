#include "pbc.h"
#include "map.h"
#include "context.h"
#include "pattern.h"
#include "proto.h"
#include "alloc.h"
#include "bootstrap.h"
#include "stringpool.h"
#include "descriptor.pbc.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

/*

// Descriptor

// google.protobuf.Descriptor.proto encoded in descriptor.pbc.h with proto pbc.file .

package pbc;

message field {
	optional string name = 1;
	optional int32 id = 2;
	optional int32 label = 3;	// 0 optional 1 required 2 repeated 
	optional int32 type = 4;	// type_id
	optional string type_name = 5;	
	optional int32 default_int = 6;	 
	optional string default_string = 7;
	optional double default_real = 8;
}

message file {
	optional string name = 1;
	repeated string dependency = 2;

	repeated string message_name = 3;
	repeated int32 message_size = 4;
	repeated field message_field = 5;

	repeated string enum_name = 6;
	repeated int32 enum_size = 7;
	repeated string enum_string = 8;
	repeated int32 enum_id = 9;
}

*/

#define FIELD_LABEL_OPTIONAL 0
#define FIELD_LABEL_REQUIRED 1
#define FIELD_LABEL_REPEATED 2
#define FIELD_LABEL_PACKED 3

struct field_t {
	pbc_var name;
	int32_t id;
	int32_t label;
	int32_t type;
	pbc_var type_name;
	int32_t default_integer;
	pbc_var default_string;
	double default_real;
};

struct file_t {
	pbc_var name;	// string
	pbc_array dependency;	// string
	pbc_array message_name;	// string
	pbc_array message_size;	// int32
	pbc_array message_field;	// field_t
	pbc_array enum_name;	// string
	pbc_array enum_size;	// int32
	pbc_array enum_string;	// string
	pbc_array enum_id;	// int32
};

static const char *
check_file_name(struct pbc_env * p , struct file_t *file) {
	if (_pbcM_sp_query(p->files, file->name->s.str)) {
		return file->name->s.str;
	}
	int sz = pbc_array_size(file->dependency); 
	int i;
	for (i=0;i<sz;i++) {
		pbc_var var;
		pbc_array_index(file->dependency,i,var);
		if (_pbcM_sp_query(p->files, var->s.str) == NULL) {
			return var->s.str;
		}
	}

	return NULL;
}

static void
set_enum_one(struct pbc_env *p, struct file_t *file, const char *name, int start, int sz) {
	struct map_kv *table = malloc(sz * sizeof(struct map_kv));
	int i;
	for (i=0;i<sz;i++) {
		pbc_var id;
		pbc_var string;
		pbc_array_index(file->enum_id, start+i, id);
		pbc_array_index(file->enum_string, start+i, string);
		table[i].id = (int)id->integer.low;
		table[i].pointer = (void *)string->s.str;
	}
	proto_push_enum(p,name,table,sz);

	free(table);
}

static void
set_enums(struct pbc_env *p, struct file_t *file) {
	int n = pbc_array_size(file->enum_size);
	int i;
	int start = 0;
	for (i=0;i<n;i++) {
		pbc_var name;
		pbc_array_index(file->enum_name,i,name);
		pbc_var var;
		pbc_array_index(file->enum_size,i,var);
		set_enum_one(p, file, name->s.str, start , (int)var->integer.low);
		start += var->integer.low;
	}
}

static void
set_default(struct _field *f, struct field_t *input) {
	switch (f->type) {
	case PTYPE_DOUBLE:
	case PTYPE_FLOAT:
		f->default_v->real = input->default_real;
		break;
	case PTYPE_STRING:
	case PTYPE_ENUM:
		memcpy(f->default_v, input->default_string , sizeof(pbc_var));
		break;
	default:
		f->default_v->integer.low = input->default_integer;
		break;
	}
}

static void
set_msg_one(struct pbc_pattern * FIELD_T, struct pbc_env *p, struct file_t *file, const char *name, int start, int sz , pbc_array queue) {
	int i;
	for (i=0;i<sz;i++) {
		pbc_var _field;
		pbc_array_index(file->message_field, start+i, _field);
		struct field_t field;

		int ret = pbc_pattern_unpack(FIELD_T, _field->m.buffer, _field->m.len, &field);
		if (ret != 0) {
			continue;
		}
		struct _field f;
		f.id = field.id;
		f.name = field.name->s.str;
		f.type = field.type;
		f.label = field.label;
		f.type_name.n = field.type_name->s.str;
		set_default(&f, &field);

		proto_push_message(p,name, &f , queue);

		// don't need to close pattern since no array
	}
	proto_init_message(p, name);
}

static void
set_msgs(struct pbc_pattern * FIELD_T, struct pbc_env *p, struct file_t *file , pbc_array queue) {
	int n = pbc_array_size(file->message_size);
	int i;
	int start = 0;
	for (i=0;i<n;i++) {
		pbc_var name;
		pbc_array_index(file->message_name,i,name);
		pbc_var sz;
		pbc_array_index(file->message_size,i,sz);
		set_msg_one(FIELD_T, p, file, name->s.str, start , (int)sz->integer.low , queue);
		start += sz->integer.low;
	}
}

static void
set_field_one(struct pbc_env *p, struct _field *f) {
	const char * type_name = f->type_name.n;
	if (f->type == PTYPE_MESSAGE) {
		f->type_name.m  = _pbcM_sp_query(p->msgs, type_name);
//		printf("MESSAGE: %s %p\n",type_name, f->type_name.m);
	} else if (f->type == PTYPE_ENUM) {
		f->type_name.e = _pbcM_sp_query(p->enums, type_name);
//		printf("ENUM: %s %p ",type_name, f->type_name.e);
		const char * str = f->default_v->s.str;
		if (str) {
			f->default_v->e.id = _pbcM_si_query(f->type_name.e->name, str);
			f->default_v->e.name = str;
//			printf("[%s %d]\n",str,f->default_v->e.id);
		} else {
			memcpy(f->default_v, f->type_name.e->default_v, sizeof(pbc_var));
//			printf("(%s %d)\n",f->default_v->e.name,f->default_v->e.id);
		}
	}
}

void
_pbcB_register_fields(struct pbc_env *p, pbc_array queue) {
	int sz = pbc_array_size(queue);
	int i;
	for (i=0;i<sz;i++) {
		pbc_var atom;
		pbc_array_index(queue,i,atom);
		struct _field * f = atom->m.buffer;
		set_field_one(p, f);
	}
}

static struct pbc_pattern *
_pattern_new(int n)
{
	size_t sz = sizeof(struct pbc_pattern) + (sizeof(struct _pattern_field) - 1) * (n-1);
	struct pbc_pattern * ret = malloc(sz);
	memset(ret, 0 , sz);
	ret->count = n;
	return ret;
}

static void
_set_string(struct _pattern_field * f) {
	f->ptype = PTYPE_STRING;
	f->ctype = CTYPE_VAR;
}

static void
_set_int32(struct _pattern_field * f) {
	f->ptype = PTYPE_INT32;
	f->ctype = CTYPE_INT32;
}

static void
_set_double(struct _pattern_field * f) {
	f->ptype = PTYPE_DOUBLE;
	f->ctype = CTYPE_DOUBLE;
}

static void
_set_message_array(struct _pattern_field *f) {
	f->ptype = PTYPE_MESSAGE;
	f->ctype = CTYPE_ARRAY;
}

static void
_set_string_array(struct _pattern_field * f) {
	f->ptype = PTYPE_STRING;
	f->ctype = CTYPE_ARRAY;
}

static void
_set_int32_array(struct _pattern_field * f) {
	f->ptype = PTYPE_INT32;
	f->ctype = CTYPE_ARRAY;
}

#define SET_PATTERN(pat , idx , pat_type, field_name , type) \
	pat->f[idx].id = idx+1 ; \
	pat->f[idx].offset = offsetof(struct pat_type, field_name);	\
	_set_##type(&pat->f[idx]);

#define F(idx,field_name,type) SET_PATTERN(FIELD_T, idx, field_t ,field_name, type)
#define D(idx,field_name,type) SET_PATTERN(FILE_T, idx, file_t ,field_name, type)

static int
register_internal(struct pbc_env * p, void *buffer, int size) {
	struct pbc_pattern * FIELD_T =  _pattern_new(8);
	F(0,name,string);
	F(1,id,int32);
	F(2,label,int32);
	F(3,type,int32);
	F(4,type_name,string);
	F(5,default_integer,int32);
	F(6,default_string,string);
	F(7,default_real,double);

	struct pbc_pattern * FILE_T =  _pattern_new(10);

	D(0,name,string);
	D(1,dependency,string_array);
	D(2,message_name,string_array);
	D(3,message_size,int32_array);
	D(4,message_field,message_array);
	D(5,enum_name,string_array);
	D(6,enum_size,int32_array);
	D(7,enum_string,string_array);
	D(8,enum_id,int32_array);

	int ret = 0;

	struct file_t file;
	int r = pbc_pattern_unpack(FILE_T, buffer, size, &file);
	if (r != 0) {
		ret = 1;
		goto _return;
	}
	if (check_file_name(p,&file)) {
		pbc_pattern_close(FILE_T, &file);
		ret = 1;
		goto _return;
	}

	_pbcM_sp_insert(p->files , file.name->s.str, _pbcS_new());

	pbc_array queue;
	pbc_array_open(queue);

	set_enums(p, &file);
	set_msgs(FIELD_T, p, &file, queue);
	_pbcB_register_fields(p, queue);

	pbc_array_close(queue);
	pbc_pattern_close(FILE_T, &file);

_return:
	free(FIELD_T);
	free(FILE_T);
	return ret;
}

void 
_pbcB_init(struct pbc_env * p) {
	register_internal(p,pbc_descriptor,sizeof(pbc_descriptor));
}
