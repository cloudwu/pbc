local protobuf = require "protobuf"

addr = io.open("../../build/addressbook.pb","rb")
buffer = addr:read "*a"
addr:close()
protobuf.register(buffer)

for i=1,1000000 do
	local buffer = protobuf.pack("tutorial.Person name id", "Alice", 123)
	protobuf.unpack("tutorial.Person name id", buffer)
--[[
	local person = {
		name = "Alice",
		id = 123,
	}

	local buffer = protobuf.encode("tutorial.Person", person)
	local t = protobuf.decode("tutorial.Person", buffer)
]]
end
