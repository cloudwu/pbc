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

module "protobuf"

local P = c._env_new()
setmetatable(_M , { __gc = function(t) c._env_delete(P) end })

function lasterror()
	return c._last_error(P)
end

local decode_type_cache = {}
local _R_meta = {}

function _R_meta:__index(key)
	local v = decode_type_cache[self._CType][key](self._CObj, key)
	self[key] = v
	return v
end

local _reader = {}

function _reader:int(key)
	return c._rmessage_integer(self , key , 0)
end

function _reader:real(key)
	return c._rmessage_real(self , key , 0)
end

function _reader:string(key)
	return c._rmessage_string(self, key , 0)
end

function _reader:bool(key)
	return c._rmessage_integer(self , key , 0) ~= 0
end

function _reader:message(key, message_type)
	local rmessage = c._rmessage_message(self , key , 0)
	if rmessage then
		local v = {
			_CObj = rmessage,
			_CType = message_type,
		}
		return setmetatable( v , _R_meta )
	end
end

function _reader:int32(key)
	return c._rmessage_int32(self, key , 0)
end

function _reader:int64(key)
	return c._rmessage_int64(self, key , 0)
end

function _reader:int_repeated(key)
	local n = c._rmessage_size(self , key)
	local ret = {}
	for i=0,n-1 do
		table.insert(ret,  c._rmessage_integer(self , key , i))
	end
	return ret
end

function _reader:real_repeated(key)
	local n = c._rmessage_size(self , key)
	local ret = {}
	for i=0,n-1 do
		table.insert(ret,  c._rmessage_real(self , key , i))
	end
	return ret
end

function _reader:string_repeated(key)
	local n = c._rmessage_size(self , key)
	local ret = {}
	for i=0,n-1 do
		table.insert(ret,  c._rmessage_string(self , key , i))
	end
	return ret
end

function _reader:bool_repeated(key)
	local n = c._rmessage_size(self , key)
	local ret = {}
	for i=0,n-1 do
		table.insert(ret,  c._rmessage_integer(self , key , i) ~= 0)
	end
	return ret
end

function _reader:message_repeated(key, message_type)
	local n = c._rmessage_size(self , key)
	local ret = {}
	for i=0,n-1 do
		local m = {
			_CObj = c._rmessage_message(self , key , i),
			_CType = message_type,
		}
		table.insert(ret, setmetatable( m , _R_meta ))
	end
	return ret
end

function _reader:int32_repeated(key)
	local n = c._rmessage_size(self , key)
	local ret = {}
	for i=0,n-1 do
		table.insert(ret,  c._rmessage_int32(self , key , i))
	end
	return ret
end

function _reader:int64_repeated(key)
	local n = c._rmessage_size(self , key)
	local ret = {}
	for i=0,n-1 do
		table.insert(ret,  c._rmessage_int64(self , key , i))
	end
	return ret
end

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
_reader[7] = function(msg) return _reader.int64 end
_reader[8] = function(msg) return _reader.int32 end
_reader[9] = _reader[5]

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
_reader[128+7] = function(msg) return _reader.int64_repeated end
_reader[128+8] = function(msg) return _reader.int32_repeated end
_reader[128+9] = _reader[128+5]

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

local _R_metagc = {
	__index = _R_meta.__index
}

function _R_metagc:__gc()
	c._rmessage_delete(self._CObj)
end

function decode( message , buffer, length)
	local rmessage = c._rmessage_new(P, message, buffer, length)
	if rmessage then
		local self = {
			_CObj = rmessage,
			_CType = message,
		}
		return setmetatable( self , _R_metagc )
	end
end

function close_decoder(self)
	c._rmessage_delete(self._CObj)
	self._CObj = nil
	setmetatable(self,nil)
end

function register( buffer)
	c._env_register(P, buffer)
end

function register_file(filename)
	local f = assert(io.open(filename , "rb"))
	local buffer = f:read "*a"
	c._env_register(P, buffer)
	f:close()
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
	int = c._wmessage_integer,
	real = c._wmessage_real,
	enum = c._wmessage_string,
	string = c._wmessage_string,
	int64 = c._wmessage_int64,
	int32 = c._wmessage_int32,
}

function _writer:bool(k,v)
	c._wmessage_integer(self, k, v and 1 or 0)
end

function _writer:message(k, v , message_type)
	local submessage = c._wmessage_message(self, k)
	encode_message(submessage, message_type, v)
end

function _writer:int_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_integer(self,k,v)
	end
end

function _writer:real_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_real(self,k,v)
	end
end

function _writer:bool_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_integer(self, k, v and 1 or 0)
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

function _writer:int32_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_int32(self,k,v)
	end
end

function _writer:int64_repeated(k,v)
	for _,v in ipairs(v) do
		c._wmessage_int64(self,k,v)
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
_writer[7] = function(msg) return _writer.int64 end
_writer[8] = function(msg) return _writer.int32 end
_writer[9] = _writer[5]

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
_writer[128+7] = function(msg) return _writer.int64_repeated end
_writer[128+8] = function(msg) return _writer.int32_repeated end
_writer[128+9] = _writer[128+5]

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

function encode( message, t , func)
	local encoder = c._wmessage_new(P, message)
	assert(encoder ,  message)
	encode_message(encoder, message, t)
	if func then
		local buffer, len = c._wmessage_buffer(encoder)
		func(buffer, len)
		c._wmessage_delete(encoder)
	else
		local s = c._wmessage_buffer_string(encoder)
		c._wmessage_delete(encoder)
		return s
	end
end

--------- unpack ----------

local _pattern_cache = {}

local pat_meta = {
	__gc = function(t)
		c._pattern_delete(t.CObj)
	end
}

local _pattern_type = {
	[1] = {"%d","i"},
	[2] = {"%F","r"},
	[3] = {"%d","b"},
	[4] = {"%d","i"},
	[5] = {"%s","s"},
	[6] = {"%s","m"},
	[7] = {"%D","x"},
	[8] = {"%d","p"},
	[128+1] = {"%d","I"},
	[128+2] = {"%F","R"},
	[128+3] = {"%d","B"},
	[128+4] = {"%d","I"},
	[128+5] = {"%s","S"},
	[128+6] = {"%s","M"},
	[128+7] = {"%D","X"},
	[128+8] = {"%D","P"},
}

_pattern_type[9] = _pattern_type[5]
_pattern_type[128+9] = _pattern_type[128+5]


local function _pattern_create(pattern)
	local iter = string.gmatch(pattern,"[^ ]+")
	local message = iter()
	local cpat = {}
	local lua = {}
	for v in iter do
		local tidx = c._env_type(P, message, v)
		local t = _pattern_type[tidx]
		assert(t,tidx)
		table.insert(cpat,v .. " " .. t[1])
		table.insert(lua,t[2])
	end
	local cobj = c._pattern_new(P, message , "@" .. table.concat(cpat," "))
	if cobj == nil then
		return
	end
	local pat = {
		CObj = cobj,
		format = table.concat(lua),
		size = 0
	}
	pat.size = c._pattern_size(pat.format)

	setmetatable(pat, pat_meta)

	return pat
end

setmetatable(_pattern_cache, {
	__mode = "v",
	__index = function(t, key)
		local v = _pattern_create(key)
		t[key] = v
		return v
	end
})

function unpack(pattern, buffer, length)
	local pat = _pattern_cache[pattern]
	return c._pattern_unpack(pat.CObj , pat.format, pat.size, buffer, length)
end

function pack(pattern, ...)
	local pat = _pattern_cache[pattern]
	return c._pattern_pack(pat.CObj, pat.format, pat.size , ...)
end

function check(typename , field)
	if field == nil then
		return c._env_type(P,typename)
	else
		return c._env_type(P,typename,field) ~=0
	end
end

local function _next(obj, prev)
	return c._rmessage_nextkey(obj, prev)
end

function key(msg)
	return _next, msg._CObj , nil
end

local function _next_pairs(msg, prev)
	local key, t = c._rmessage_nextkey(msg._CObj, prev)
	if key == nil then
		return
	end
	return key, msg[key]
end

function _R_meta:__pairs()
	return _next_pairs , self , nil
end

_R_metagc.__pairs = _R_meta.__pairs
