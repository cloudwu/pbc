## Install

Make and install protobuf.so ( or protobuf.dll in windows ) and protobuf.lua into your lua path.

## Register

```Lua
pb = require "protobuf"

pb.register_file "addressbook.pb"
```
or

```Lua
file = io.open("addressbook.pb","rb")
buffer = file:read "*a"
file:close()

pb.register(buffer)
```

## Lua parser

You can use the lua parser to register .proto file instead of .pb file .

```Lua
parser = require "parser"

parser.register(filename [,path])
-- filename can be a string (single file) or a table (filename set)

```

See testparser.lua for detail .

parser.lua use Lpeg (http://www.inf.puc-rio.br/~roberto/lpeg/) to parser .proto file.

## Message Mode
```Lua
pb = require "protobuf"

pb.register "addressbook.pb"

stringbuffer = pb.encode("tutorial.Person", 
  {
    name = "Alice",
    id = 12345,
    phone = {
      {
        number = "87654321"
      },
    }
  })

-- If you want to get a lightuserdata(C pointer) and a length

pb.encode("tutorial.Person",
  {
    name = "Alice",
    id = 12345,
    phone = {
      {
        number = "87654321"
      },
    }
  },
  function (pointer, length)
    -- do something
  end)
```

For decode :

```Lua
result = pb.decode("tutorial.Person", stringbuffer)
-- decode also support lightuserdata and length of data instead of a string :
-- pb.decode("tutorial.Person", buffer, length)
```

## Pattern mode

Pattern mode is cheaper than message mode.

```Lua
phone = pb.pack("tutorial.Person.PhoneNumber number","87654321")  -- pack a PhoneNumber package.
person = pb.pack("tutorial.Person name id phone","Alice",123,{phone}) -- phone list is a repeated field

-- use pb.unpack to unpack package

name, id, phone_package = pb.unpack("tutorial.Person name id phone", person)
number = pb.unpack("tutorial.Person.PhoneNumber number",unpack(phone_package[1])) -- unpack return message with { buffer, length }
```

## Other API

pb.check(typename , field) can check the field of typename exist.

or you can use pb.check(typename) to check the typename registered.

pb.lasterror() will return an internal error string .
