.PHONY : all clean

TARGET = protobuf.so
CC = gcc
CFLAGS = -O2 -fPIC -Wall

all : $(TARGET)


$(TARGET) : pbc-lua53.c
	$(CC) $(CFLAGS) --shared -o $@ -I../.. -L../../build $^ -lpbc

clean :
	rm $(TARGET)
