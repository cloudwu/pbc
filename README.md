## PBC

PBC is a google protocol buffers library for C without code generation.

## Message API

You can use *wmessage* for encoding , and *rmessage* for decoding.

See test_addressbook.c for detail.

## Pattern API

If you need higher performance , you can use pbc_pattern_xxx api .

See test_pattern.c for detail.

pattern api is faster, and less memory used, and you can access data from native C struct . 

## Extension

PBC support extension with a simple way . It add prefix to the extension field name. 

## Service

Not supported

## Enum

With message API , you can use both string and integer as the enum type , and with pattern api it must be integer. 

* http://blog.codingnow.com/2011/12/protocol_buffers_for_c.html (in Chinese)