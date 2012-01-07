.PHONY : all 51 clean

all :
	gcc -O2 -Wall --shared -o protobuf.dll -I../.. -I/usr/local/include -L/usr/local/bin -L../../build pbc-lua.c -lpbc -llua52

51 :
#	gcc -O2 -Wall --shared -o protobuf.dll -I../.. -I/d/project/luajit-2.0/src -L/usr/local/bin -L../../build pbc-lua.c -lpbc -llua51
	gcc -O2 -Wall --shared -o protobuf.dll -I../.. -I/d/project/lua-5.1.4/src -L/usr/local/bin -L../../build pbc-lua.c -lpbc -llua51

clean :
	rm protobuf.dll