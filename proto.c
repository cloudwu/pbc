#include "pbc.h"
#include "proto.h"
#include "pattern.h"
#include "map.h"
#include "alloc.h"
#include "stringpool.h"
#include "bootstrap.h"

#include <stdlib.h>
#include <string.h>

struct _message * 
_pbcP_get_message(struct pbc_env * p , const char *name) {
	return _pbcM_sp_query(p->msgs, name);
}

struct pbc_env * 
pbc_new(void) {
	struct pbc_env * p = malloc(sizeof(*p));
	p->files = _pbcM_sp_new();
	p->enums = _pbcM_sp_new();
	p->msgs = _pbcM_sp_new();

	_pbcB_init(p);

	return p;
}

static void
free_enum(void *p) {
	struct _enum * e = p;

	_pbcM_ip_delete(e->id);
	_pbcM_si_delete(e->name);

	free(p);
}

static void
free_stringpool(void *p) {
	_pbcS_delete(p);
}

static void
free_msg(void *p) {
	struct _message * m = p;
	if (m->id)
		_pbcM_ip_delete(m->id);
	free(m->def);
	_pbcM_sp_foreach(m->name, free);
	_pbcM_sp_delete(m->name);
	free(p);
}

void 
pbc_delete(struct pbc_env *p) {
	_pbcM_sp_foreach(p->enums, free_enum);
	_pbcM_sp_delete(p->enums);

	_pbcM_sp_foreach(p->msgs, free_msg);
	_pbcM_sp_delete(p->msgs);

	_pbcM_sp_foreach(p->files, free_stringpool);
	_pbcM_sp_delete(p->files);

	free(p);
}

struct _enum *
_pbcP_push_enum(struct pbc_env * p, const char *name, struct map_kv *table, int sz) {
	void * check = _pbcM_sp_query(p->enums, name);
	if (check)
		return NULL;
	struct _enum * v = malloc(sizeof(*v));
	v->key = name;
	v->id = _pbcM_ip_new(table,sz);
	v->name = _pbcM_si_new(table,sz);
	v->default_v->e.id = table[0].id;
	v->default_v->e.name = table[0].pointer;

	_pbcM_sp_insert(p->enums, name , v);
	return v;
}

void 
_pbcP_push_message(struct pbc_env * p, const char *name, struct _field *f , pbc_array queue) {
	struct _message * m = _pbcM_sp_query(p->msgs, name);
	if (m==NULL) {
		m = malloc(sizeof(*m));
		m->def = NULL;
		m->key = name;
		m->id = NULL;
		m->name = _pbcM_sp_new();
		_pbcM_sp_insert(p->msgs, name, m);
	}
	struct _field * field = malloc(sizeof(*field));
	memcpy(field,f,sizeof(*f));
	_pbcM_sp_insert(m->name, field->name, field); 
	pbc_var atom;
	atom->m.buffer = field;
	if (f->type == PTYPE_MESSAGE || f->type == PTYPE_ENUM) {
		_pbcA_push(queue, atom);
	}
}

struct _iter {
	int count;
	struct map_kv * table;
};

static void
_count(void *p, void *ud) {
	struct _iter *iter = ud;
	iter->count ++;
}

static void
_set_table(void *p, void *ud) {
	struct _field * field = p;
	struct _iter *iter = ud;
	iter->table[iter->count].id = field->id;
	iter->table[iter->count].pointer = field;
	++iter->count;
}

struct _message * 
_pbcP_init_message(struct pbc_env * p, const char *name) {
	struct _message * m = _pbcM_sp_query(p->msgs, name);
	if (m == NULL) {
		return NULL;
	}
	if (m->id) {
		return NULL;
	}
	struct _iter iter = { 0, NULL };
	_pbcM_sp_foreach_ud(m->name, _count, &iter);
	iter.table = malloc(iter.count * sizeof(struct map_kv));
	iter.count = 0;
	_pbcM_sp_foreach_ud(m->name, _set_table, &iter);

	m->id = _pbcM_ip_new(iter.table , iter.count);

	free(iter.table);

	return m;
}

int 
_pbcP_message_default(struct _message * m, const char * name, pbc_var defv) {
	struct _field * f= _pbcM_sp_query(m->name, name);
	if (f==NULL) {
		// invalid key
		defv->p[0] = NULL;
		defv->p[1] = NULL;
		return -1;
	}
	*defv = *(f->default_v);
	return f->type;
}

