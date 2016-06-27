protobuf = require "protobuf"
parser = require "parser"

t = parser.register("addressbook.proto","../../test")

local addressbook1 = {
	name = "Alice",
	id = 12345,
	phone = {
		{ number = "1301234567" },
		{ number = "87654321", type = "WORK" },
		{ number = "13912345678", type = "MOBILE" },
	},
	email = "username@domain.com"
}

local addressbook2 = {
	name = "Bob",
	id = 12346,
	phone = {
		{ number = "1301234568" },
		{ number = "98765432", type = "HOME" },
		{ number = "13998765432", type = "MOBILE" },
	}
}


code1 = protobuf.encode("tutorial.Person", addressbook1)
code2 = protobuf.encode("tutorial.Person", addressbook2)

decode1 = protobuf.decode("tutorial.Person" , code1)

-- BUG [ISSUE#27](https://github.com/cloudwu/pbc/issues/27)
decode1.profile.nick_name = "AHA"
decode1.profile.icon = "id:1"

decode2 = protobuf.decode("tutorial.Person" , code2)

function print_addr(decoded)
	print(string.format('ID: %d, Name: %s, Email: %s', decoded.id, decoded.name, tostring(decoded.email)))
	if decoded.profile then
		print(string.format('\tNickname: %s, Icon: %s', tostring(decoded.profile.nick_name), tostring(decoded.profile.icon)))
	end
	for k, v in ipairs(decoded.phone) do
		print(string.format("\tPhone NO.%s: %16s %s", k, v.number, tostring(v.type)))
	end
end

print_addr(decode1)
print_addr(decode2)

buffer = protobuf.pack("tutorial.Person name id", "Alice", 123)
print(protobuf.unpack("tutorial.Person name id", buffer))

code_phone = protobuf.encode("tutorial.Person.PhoneNumber", { number = "18612345678" })
decode_phone = protobuf.decode("tutorial.Person.PhoneNumber" , code_phone)
print(string.format("Phone: %16s %s", decode_phone.number, tostring(decode_phone.type)))