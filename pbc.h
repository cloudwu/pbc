#ifndef PROTOBUF_C_H
#define PROTOBUF_C_H

#include <stdio.h>
#include <stdint.h>

#define PBC_ARRAY_CAP 64

typedef struct _pbc_array { char _data[PBC_ARRAY_CAP]; } pbc_array[1];

struct pbc_pattern;
struct pbc_env;

struct pbc_env * pbc_new(void);
void pbc_delete(struct pbc_env *);
int pbc_register(struct pbc_env *, void *buffer, int size);

// message api

struct pbc_rmessage * pbc_rmessage_new(struct pbc_env * env, const char * typename , void *buffer, int size);
void pbc_rmessage_delete(struct pbc_rmessage *);

uint32_t pbc_rmessage_integer(struct pbc_rmessage * , const char *key , int index, uint32_t *hi);
double pbc_rmessage_real(struct pbc_rmessage * , const char *key , int index);
const char * pbc_rmessage_string(struct pbc_rmessage * , const char *key , int index, int *sz);
struct pbc_rmessage * pbc_rmessage_message(struct pbc_rmessage *, const char *key, int index);
int pbc_rmessage_size(struct pbc_rmessage *, const char *key);

struct pbc_wmessage * pbc_wmessage_new(struct pbc_env * env, const char *typename);
void pbc_wmessage_delete(struct pbc_wmessage *);

void pbc_wmessage_integer(struct pbc_wmessage *, const char *key, uint32_t low, uint32_t hi);
void pbc_wmessage_real(struct pbc_wmessage *, const char *key, double v);
void pbc_wmessage_string(struct pbc_wmessage *, const char *key, const char * v, int len);
struct pbc_wmessage * pbc_wmessage_message(struct pbc_wmessage *, const char *key);
void * pbc_wmessage_buffer(struct pbc_wmessage *, int *sz);


// pattern api

struct longlong {
	uint32_t low;
	uint32_t hi;
};

typedef union _pbc_var {
	struct longlong integer;
	double real;
	struct {
		const char * str;
		int len;
	} s;
	struct {
		int id;
		const char * name;
	} e;
	struct {
		void * buffer;
		int len;
	} m;
	void * p[2];
} pbc_var[1];

struct pbc_rmessage;
struct pbc_wmessage;

void pbc_array_open(pbc_array);
void pbc_array_close(pbc_array);

void pbc_array_push(pbc_array, pbc_var var);
void pbc_array_index(pbc_array , int idx, pbc_var var);

int pbc_array_size(pbc_array);

int pbc_pattern_unpack(struct pbc_pattern *, void *buffer, int sz , void * output);
void pbc_pattern_close(struct pbc_pattern *, void * data);

#endif
