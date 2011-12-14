.PHONY : all

all :
	gcc -O2 -Wall --shared -o protobuf.dll -I../.. -I/usr/local/include -L/usr/local/bin -L../../build pbc-lua.c -lpbc -llua52
#	gcc -O2 -Wall --shared -o protobuf.dll -I../.. -I/d/project/luajit-2.0/src -L/usr/local/bin -L../../build pbc-lua.c -lpbc -llua51