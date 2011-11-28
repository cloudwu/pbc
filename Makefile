.PHONY : all test clean

all : test

test : test_pbc.exe test_array.exe test_varint.exe test_map.exe

clean :
	rm -f *.exe *.o

test_varint.exe : test_varint.c varint.c
	gcc -g -Wall -o $@ $^

test_map.exe : test_map.c map.c alloc.c
	gcc -g -Wall -o $@ $^

test_pbc.exe : test_pbc.c pbc.c varint.c array.c pattern.c register.c proto.c map.c alloc.c rmessage.c wmessage.c bootstrap.c stringpool.c
	gcc -g -Wall -o $@ $^

test_array.exe : test_array.c array.c alloc.c
	gcc -g -Wall -o $@ $^
	