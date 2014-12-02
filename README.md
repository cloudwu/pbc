## PBC

PBC is a google protocol buffers library for C without code generation.

## Quick Example

    package tutorial;
    
    message Person {
      required string name = 1;
      required int32 id = 2;        // Unique ID number for this person.
      optional string email = 3;
    
      enum PhoneType {
        MOBILE = 0;
        HOME = 1;
        WORK = 2;
      }
    
      message PhoneNumber {
        required string number = 1;
        optional PhoneType type = 2 [default = HOME];
      }
    
      repeated PhoneNumber phone = 4;
    }

```C
struct pbc_rmessage * m = pbc_rmessage_new(env, "tutorial.Person", slice);
printf("name = %s\n", pbc_rmessage_string(m , "name" , 0 , NULL));
printf("id = %d\n", pbc_rmessage_integer(m , "id" , 0 , NULL));
printf("email = %s\n", pbc_rmessage_string(m , "email" , 0 , NULL));

int phone_n = pbc_rmessage_size(m, "phone");
int i;

for (i=0;i<phone_n;i++) {
	struct pbc_rmessage * p = pbc_rmessage_message(m , "phone", i);
	printf("\tnumber[%d] = %s\n",i,pbc_rmessage_string(p , "number", i ,NULL));
	printf("\ttype[%d] = %s\n",i,pbc_rmessage_string(p, "type", i, NULL));
}

pbc_rmessage_delete(m);
```

## Message API

You can use *wmessage* for encoding , and *rmessage* for decoding.

See test/addressbook.c for details.

## Pattern API

If you need better performance , you can use pbc_pattern_xxx api .

See test/pattern.c for details.

Pattern api is faster and less memory used because it can access data in native C struct.

## Extension

PBC support extension in a very simple way . PBC add a specific prefix to every extension field name. 

## Service

Not supported

## Enum

With message API , you can use both string and integer as enum type . They must be integer in Pattern API. 

## Lua bindings

cd bindings/lua && make

See https://github.com/cloudwu/pbc/tree/master/binding/lua/README.md

## Question ?

* Send me email : http://www.codingnow.com/2000/gmail.gif
* My Blog : http://blog.codingnow.com
* Design : http://blog.codingnow.com/2011/12/protocol_buffers_for_c.html (in Chinese)
* Build for Visual Studio 2012 : https://github.com/miaodadao/pbc


