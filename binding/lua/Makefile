CC = gcc
CFLAGS = -O2 -fPIC -Wall
LUADIR = /usr/local/include
TARGET = protobuf.so
SHARED = -shared

.PHONY : all clean

all : $(TARGET)

$(TARGET) : pbc-lua.c
	$(CC) $(CFLAGS) $(SHARED) -o $@ -I../.. -I$(LUADIR) -L../../build $^ -lpbc

clean :
	rm -f $(TARGET)
