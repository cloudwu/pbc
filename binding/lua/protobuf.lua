local c = require "protobuf.c"
local setmetatable = setmetatable
local type = type
local table = table
local assert = assert
local pairs = pairs
local ipairs = ipairs
local print = print

module "protobuf"

local P = c._env_new()
setmetatable(_ENV , { __gc = function(t) c._env_delete(P) end })

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

local _decode_type_meta = {}

function _decode_type_meta:__index(key)
	local t, msg = c._env_type(P, self._CType, key)
	local func = _reader[t](msg)
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

function register( buffer , length)
	c._env_register(P, buffer, length)
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

local _encode_type_meta = {}

function _encode_type_meta:__index(key)
	local t, msg = c._env_type(P, self._CType, key)
	local func = _writer[t](msg)
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
	assert(encoder , "Unknown message : " .. message)
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
