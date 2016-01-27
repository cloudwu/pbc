local c = require "protobuf.c"

local setmetatable = setmetatable
local type = type
local table = table
local assert = assert
local pairs = pairs
local ipairs = ipairs
local string = string
local print = print
local io = io
local tinsert = table.insert
local rawget = rawget

local M = {}

local _pattern_cache = {}

local P,GC

P = debug.getregistry().PROTOBUF_ENV

if P then
	GC = c._gc()
else
	P= c._env_new()
	GC = c._gc(P)
end

M.GC = GC

function M.lasterror()
	return c._last_error(P)
end

local decode_type_cache = {}
local _R_meta = {}

function _R_meta:__index(key)
	local v = decode_type_cache[self._CType][key](self, key)
	self[key] = v
	return v
end

local _reader = {}

function _reader:real(key)
	return c._rmessage_real(self._CObj , key , 0)
end

function _reader:string(key)
	return c._rmessage_string(self._CObj , key , 0)
end

function _reader:bool(key)
	return c._rmessage_int(self._CObj , key , 0) ~= 0
end

function _reader:message(key, message_type)
	local rmessage = c._rmessage_message(self._CObj , key , 0)
	if rmessage then
		local v = {
			_CObj = rmessage,
			_CType = message_type,
			_Parent = self,
		}
		return setmetatable( v , _R_meta )
	end
end

function _reader:int(key)
	return c._rmessage_int(self._CObj , key , 0)
end

function _reader:real_repeated(key)
	local cobj = self._CObj
	local n = c._rmessage_size(cobj , key)
	local ret = {}
	for i=0,n-1 do
		tinsert(ret,  c._rmessage_real(cobj , key , i))
	end
	return ret
end

function _reader:string_repeated(key)
	local cobj = self._CObj
	local n = c._rmessage_size(cobj , key)
	local ret = {}
	for i=0,n-1 do
		tinsert(ret,  c._rmessage_string(cobj , key , i))
	end
	return ret
end

function _reader:bool_repeated(key)
	local cobj = self._CObj
	local n = c._rmessage_size(cobj , key)
	local ret = {}
	for i=0,n-1 do
		tinsert(ret,  c._rmessage_int(cobj , key , i) ~= 0)
	end
	return ret
end

function _reader:message_repeated(key, message_type)
	local cobj = self._CObj
	local n = c._rmessage_size(cobj , key)
	local ret = {}
	for i=0,n-1 do
		local m = {
			_CObj = c._rmessage_message(cobj , key , i),
			_CType = message_type,
			_Parent = self,
		}
		tinsert(ret, setmetatable( m , _R_meta ))
	end
	return ret
end

function _reader:int_repeated(key)
	local cobj = self._CObj
	local n = c._rmessage_size(cobj , key)
	local ret = {}
	for i=0,n-1 do
		tinsert(ret,  c._rmessage_int(cobj , key , i))
	end
	return ret
end

--[[
#define PBC_INT 1
#define PBC_REAL 2
#define PBC_BOOL 3
#define PBC_ENUM 4
#define PBC_STRING 5
#define PBC_MESSAGE 6
#define PBC_FIXED64 7
#define PBC_FIXED32 8
#define PBC_BYTES 9
#define PBC_INT64 10
#define PBC_UINT 11
#define PBC_UNKNOWN 12
#define PBC_REPEATED 128
]]

_reader[1] = function(msg) return _reader.int end
_reader[2] = function(msg) return _reader.real end
_reader[3] = function(msg) return _reader.bool end
_reader[4] = function(msg) return _reader.string end
_reader[5] = function(msg) return _reader.string end
_reader[6] = function(msg)
	local message = _reader.message
	return	function(self,key)
			return message(self, key, msg)
		end
end
_reader[7] = _reader[1]
_reader[8] = _reader[1]
_reader[9] = _reader[5]
_reader[10] = _reader[7]
_reader[11] = _reader[7]

_reader[128+1] = function(msg) return _reader.int_repeated end
_reader[128+2] = function(msg) return _reader.real_repeated end
_reader[128+3] = function(msg) return _reader.bool_repeated end
_reader[128+4] = function(msg) return _reader.string_repeated end
_reader[128+5] = function(msg) return _reader.string_repeated end
_reader[128+6] = function(msg)
	local message = _reader.message_repeated
	return	function(self,key)
			return message(self, key, msg)
		end
end
_reader[128+7] = _reader[128+1]
_reader[128+8] = _reader[128+1]
_reader[128+9] = _reader[128+5]
_reader[128+10] = _reader[128+7]
_reader[128+11] = _reader[128+7]

local _decode_type_meta = {}

function _decode_type_meta:__index(key)
	local t, msg = c._env_type(P, self._CType, key)
	local func = assert(_reader[t],key)(msg)
	self[key] = func
	return func
end

setmetatable(decode_type_cache , {
	__index = function(self, key)
		local v = setmetatable({ _CType = key } , _decode_type_meta)
		self[key] = v
		return v
	end
})

local function decode_message( message , buffer, length)
	local rmessage = c._rmessage_new(P, message, buffer, length)
	if rmessage then
		local self = {
			_CObj = rmessage,
			_CType = message,
		}
		c._add_rmessage(GC,rmessage)
		return setmetatable( self , _R_meta )
	end
end

----------- encode ----------------

local encode_type_cache = {}

local function encode_message(CObj, message_type, t)
	local type = encode_type_cache[message_type]
	for k,v in pairs(t) do
		local func = type[k]
		func(CObj, k , v)
	end
end

local _writer = {
	real = c._wmessage_real,
	enum = c._wmessage_string,
	string = c._wmessage_string,
	int = c._wmessage_int,
}

function _writer:bool(k,v)
	c._wmessage_int(self, k, v and 1 or 0)
end

function _writer:message(k, v , message_type)
	local submessage = c._wmessage_message(self, k)
	encode_message(submessage, message_type, v)
end

function _writer:real_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_real(self,k,v)
	end
end

function _writer:bool_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_int(self, k, v and 1 or 0)
	end
end

function _writer:string_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_string(self,k,v)
	end
end

function _writer:message_repeated(k,v, message_type)
	for _,v in ipairs(v) do
		local submessage = c._wmessage_message(self, k)
		encode_message(submessage, message_type, v)
	end
end

function _writer:int_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_int(self,k,v)
	end
end

_writer[1] = function(msg) return _writer.int end
_writer[2] = function(msg) return _writer.real end
_writer[3] = function(msg) return _writer.bool end
_writer[4] = function(msg) return _writer.string end
_writer[5] = function(msg) return _writer.string end
_writer[6] = function(msg)
	local message = _writer.message
	return	function(self,key , v)
			return message(self, key, v, msg)
		end
end
_writer[7] = _writer[1]
_writer[8] = _writer[1]
_writer[9] = _writer[5]
_writer[10] = _writer[7]
_writer[11] = _writer[7]

_writer[128+1] = function(msg) return _writer.int_repeated end
_writer[128+2] = function(msg) return _writer.real_repeated end
_writer[128+3] = function(msg) return _writer.bool_repeated end
_writer[128+4] = function(msg) return _writer.string_repeated end
_writer[128+5] = function(msg) return _writer.string_repeated end
_writer[128+6] = function(msg)
	local message = _writer.message_repeated
	return	function(self,key, v)
			return message(self, key, v, msg)
		end
end

_writer[128+7] = _writer[128+1]
_writer[128+8] = _writer[128+1]
_writer[128+9] = _writer[128+5]
_writer[128+10] = _writer[128+7]
_writer[128+11] = _writer[128+7]

local _encode_type_meta = {}

function _encode_type_meta:__index(key)
	local t, msg = c._env_type(P, self._CType, key)
	local func = assert(_writer[t],key)(msg)
	self[key] = func
	return func
end

setmetatable(encode_type_cache , {
	__index = function(self, key)
		local v = setmetatable({ _CType = key } , _encode_type_meta)
		self[key] = v
		return v
	end
})

function M.encode( message, t , func , ...)
	local encoder = c._wmessage_new(P, message)
	assert(encoder ,  message)
	encode_message(encoder, message, t)
	if func then
		local buffer, len = c._wmessage_buffer(encoder)
		local ret = func(buffer, len, ...)
		c._wmessage_delete(encoder)
		return ret
	else
		local s = c._wmessage_buffer_string(encoder)
		c._wmessage_delete(encoder)
		return s
	end
end

--------- unpack ----------

local _pattern_type = {
	[1] = {"%d","i"},
	[2] = {"%F","r"},
	[3] = {"%d","b"},
	[5] = {"%s","s"},
	[6] = {"%s","m"},
	[7] = {"%D","d"},
	[128+1] = {"%a","I"},
	[128+2] = {"%a","R"},
	[128+3] = {"%a","B"},
	[128+5] = {"%a","S"},
	[128+6] = {"%a","M"},
	[128+7] = {"%a","D"},
}

_pattern_type[4] = _pattern_type[1]
_pattern_type[8] = _pattern_type[1]
_pattern_type[9] = _pattern_type[5]
_pattern_type[10] = _pattern_type[7]
_pattern_type[11] = _pattern_type[7]
_pattern_type[128+4] = _pattern_type[128+1]
_pattern_type[128+8] = _pattern_type[128+1]
_pattern_type[128+9] = _pattern_type[128+5]
_pattern_type[128+10] = _pattern_type[128+7]
_pattern_type[128+11] = _pattern_type[128+7]


local function _pattern_create(pattern)
	local iter = string.gmatch(pattern,"[^ ]+")
	local message = iter()
	local cpat = {}
	local lua = {}
	for v in iter do
		local tidx = c._env_type(P, message, v)
		local t = _pattern_type[tidx]
		assert(t,tidx)
		tinsert(cpat,v .. " " .. t[1])
		tinsert(lua,t[2])
	end
	local cobj = c._pattern_new(P, message , "@" .. table.concat(cpat," "))
	if cobj == nil then
		return
	end
	c._add_pattern(GC, cobj)
	local pat = {
		CObj = cobj,
		format = table.concat(lua),
		size = 0
	}
	pat.size = c._pattern_size(pat.format)

	return pat
end

setmetatable(_pattern_cache, {
	__index = function(t, key)
		local v = _pattern_create(key)
		t[key] = v
		return v
	end
})

function M.unpack(pattern, buffer, length)
	local pat = _pattern_cache[pattern]
	return c._pattern_unpack(pat.CObj , pat.format, pat.size, buffer, length)
end

function M.pack(pattern, ...)
	local pat = _pattern_cache[pattern]
	return c._pattern_pack(pat.CObj, pat.format, pat.size , ...)
end

function M.check(typename , field)
	if field == nil then
		return c._env_type(P,typename)
	else
		return c._env_type(P,typename,field) ~=0
	end
end

--------------

local default_cache = {}

-- todo : clear default_cache, v._CObj

local function default_table(typename)
	local v = default_cache[typename]
	if v then
		return v
	end

	v = { __index = assert(decode_message(typename , "")) }

	default_cache[typename]  = v
	return v
end

local decode_message_mt = {}

local function decode_message_cb(typename, buffer)
	return setmetatable ( { typename, buffer } , decode_message_mt)
end

function M.decode(typename, buffer, length)
	local ret = {}
	local ok = c._decode(P, decode_message_cb , ret , typename, buffer, length)
	if ok then
		return setmetatable(ret , default_table(typename))
	else
		return false , c._last_error(P)
	end
end

local function expand(tbl)
	local typename = rawget(tbl , 1)
	local buffer = rawget(tbl , 2)
	tbl[1] , tbl[2] = nil , nil
	assert(c._decode(P, decode_message_cb , tbl , typename, buffer), typename)
	setmetatable(tbl , default_table(typename))
end

function decode_message_mt.__index(tbl, key)
	expand(tbl)
	return tbl[key]
end

function decode_message_mt.__pairs(tbl)
	expand(tbl)
	return pairs(tbl)
end

local function set_default(typename, tbl)
	for k,v in pairs(tbl) do
		if type(v) == "table" then
			local t, msg = c._env_type(P, typename, k)
			if t == 6 then
				set_default(msg, v)
			elseif t == 128+6 then
				for _,v in ipairs(v) do
					set_default(msg, v)
				end
			end
		end
	end
	return setmetatable(tbl , default_table(typename))
end

function M.register(buffer)
	c._env_register(P, buffer)
end

function M.register_file(filename)
	local f = assert(io.open(filename , "rb"))
	local buffer = f:read "*a"
	c._env_register(P, buffer)
	f:close()
end

function M.enum_id(enum_type, enum_name)
	return c._env_enum_id(P, enum_type, enum_name)
end

function M.extract(tbl)
    local typename = rawget(tbl , 1)
    local buffer = rawget(tbl , 2)
    if type(typename) == "string" and type(buffer) == "string" then
        if M.check(typename) then
            expand(tbl)
        end
    end

    for k, v in pairs(tbl) do
        if type(v) == "table" then
            M.extract(v)
        end
    end
end

M.default=set_default

return M
