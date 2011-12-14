#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "pbc.h"

static int
_env_new(lua_State *L) {
	struct pbc_env * env = pbc_new();
	lua_pushlightuserdata(L, env);
	return 1;
}

static int
_env_delete(lua_State *L) {
	struct pbc_env * env = lua_touserdata(L,1);
	pbc_delete(env);

	return 0;
}

static int
_env_register(lua_State *L) {
	struct pbc_env * env = lua_touserdata(L,1);
	size_t sz = 0;
	const char * buffer = lua_tolstring(L, 2 , &sz);
	struct pbc_slice slice;
	slice.buffer = (void *)buffer;
	slice.len = (int)sz;
	int ret = pbc_register(env, &slice);

	if (ret) {
		return luaL_error(L, "register fail");
	}
	return 0;
}

static int
_rmessage_new(lua_State *L) {
	struct pbc_env * env = lua_touserdata(L,1);
	const char * typename = lua_tostring(L,2);
	struct pbc_slice slice;
	if (lua_isstring(L,3)) {
		size_t sz = 0;
		slice.buffer = (void *)lua_tolstring(L,3,&sz);
		slice.len = (int)sz;
	} else {
		slice.buffer = lua_touserdata(L,3);
		slice.len = lua_tointeger(L,4);
	}
	struct pbc_rmessage * m = pbc_rmessage_new(env, typename, &slice);
	if (m==NULL)
		return 0;
	lua_pushlightuserdata(L,m);
	return 1;
}

static int
_rmessage_delete(lua_State *L) {
	struct pbc_rmessage * m = lua_touserdata(L,1);
	pbc_rmessage_delete(m);

	return 0;
}

static int
_rmessage_integer(lua_State *L) {
	struct pbc_rmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	int index = lua_tointeger(L,3);
	uint32_t v = pbc_rmessage_integer(m, key, index, NULL);

	lua_pushinteger(L,v);

	return 1;
}

static int
_rmessage_int64(lua_State *L) {
	struct pbc_rmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	int index = lua_tointeger(L,3);
	uint32_t v[2];
	v[0] = pbc_rmessage_integer(m, key, index, &v[1]);

	lua_pushlstring(L,(const char *)v,sizeof(v));

	return 1;
}

static int 
_rmessage_real(lua_State *L) {
	struct pbc_rmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	int index = lua_tointeger(L,3);
	double v = pbc_rmessage_real(m, key, index);

	lua_pushnumber(L,v);

	return 1;
}

static int
_rmessage_string(lua_State *L) {
	struct pbc_rmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	int index = lua_tointeger(L,3);
	int sz = 0;
	const char * v = pbc_rmessage_string(m,key,index,&sz);
	lua_pushlstring(L,v,sz);
	return 1;
}

static int
_rmessage_message(lua_State *L) {
	struct pbc_rmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	int index = lua_tointeger(L,3);
	struct pbc_rmessage * v = pbc_rmessage_message(m,key,index);
	lua_pushlightuserdata(L,v);
	return 1;
}

static int
_rmessage_size(lua_State *L) {
	struct pbc_rmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);

	int sz = pbc_rmessage_size(m, key);

	lua_pushinteger(L, sz);

	return 1;
}

static int
_env_type(lua_State *L) {
	struct pbc_env * env = lua_touserdata(L,1);
	const char * typename = lua_tostring(L,2);
	const char * key = lua_tostring(L,3);
	const char * type = NULL;
	int ret = pbc_type(env, typename, key, &type);
	lua_pushinteger(L,ret);
	if (type == NULL) {
		return 1;
	} {
		lua_pushstring(L, type);
		return 2;
	}
}

static int
_wmessage_new(lua_State *L) {
	struct pbc_env * env = lua_touserdata(L,1);
	const char * typename = lua_tostring(L,2);
	struct pbc_wmessage * ret = pbc_wmessage_new(env, typename);
	lua_pushlightuserdata(L,ret);
	return 1;
}

static int
_wmessage_delete(lua_State *L) {
	struct pbc_wmessage * m = lua_touserdata(L,1);
	pbc_wmessage_delete(m);

	return 0;
}


static int
_wmessage_integer(lua_State *L) {
	struct pbc_wmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	int number = lua_tointeger(L,3);
	uint32_t hi = 0;
	if (number < 0)
		hi = ~0;
	pbc_wmessage_integer(m, key, number, hi);

	return 0;
}

static int
_wmessage_real(lua_State *L) {
	struct pbc_wmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	double number = lua_tonumber(L,3);
	pbc_wmessage_real(m, key, number);

	return 0;
}

static int
_wmessage_string(lua_State *L) {
	struct pbc_wmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	size_t len = 0;
	const char * v = lua_tolstring(L,3,&len);
	pbc_wmessage_string(m, key, v, (int)len);

	return 0;
}

static int
_wmessage_message(lua_State *L) {
	struct pbc_wmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	struct pbc_wmessage * ret = pbc_wmessage_message(m, key);
	lua_pushlightuserdata(L, ret);

	return 1;
}

static int
_wmessage_int64(lua_State *L) {
	struct pbc_wmessage * m = lua_touserdata(L,1);
	const char * key = lua_tostring(L,2);
	const char * number = lua_tostring(L,3);
	const uint32_t * v = (const uint32_t *) number;
	pbc_wmessage_integer(m, key, v[0] , v[1]);
	return 0;
}

static int
_wmessage_buffer(lua_State *L) {
	struct pbc_slice slice;
	struct pbc_wmessage * m = lua_touserdata(L,1);
	pbc_wmessage_buffer(m , &slice);
	lua_pushlightuserdata(L, slice.buffer);
	lua_pushinteger(L, slice.len);
	return 2;
}

static int
_wmessage_buffer_string(lua_State *L) {
	struct pbc_slice slice;
	struct pbc_wmessage * m = lua_touserdata(L,1);
	pbc_wmessage_buffer(m , &slice);
	lua_pushlstring(L, (const char *)slice.buffer, slice.len);
	return 1;
}

int
luaopen_protobuf_c(lua_State *L) {
	luaL_Reg reg[] = {
		{"_env_new" , _env_new },
		{"_env_delete" , _env_delete },
		{"_env_register" , _env_register },
		{"_env_type", _env_type },
		{"_rmessage_new" , _rmessage_new },
		{"_rmessage_delete" , _rmessage_delete },
		{"_rmessage_integer" , _rmessage_integer },
		{"_rmessage_int64", _rmessage_int64 },
		{"_rmessage_real" , _rmessage_real },
		{"_rmessage_string" , _rmessage_string },
		{"_rmessage_message" , _rmessage_message },
		{"_rmessage_size" , _rmessage_size },
		{"_wmessage_new", _wmessage_new },
		{"_wmessage_delete", _wmessage_delete },
		{"_wmessage_integer", _wmessage_integer },
		{"_wmessage_real", _wmessage_real },
		{"_wmessage_string", _wmessage_string },
		{"_wmessage_message", _wmessage_message },
		{"_wmessage_int64", _wmessage_int64 },
		{"_wmessage_buffer", _wmessage_buffer },
		{"_wmessage_buffer_string", _wmessage_buffer_string },
		{NULL,NULL},
	};

	luaL_newlib(L, reg);

	return 1;
}
