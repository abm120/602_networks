CFLAGS+=-g -Wall
LDFLAGS+=-g -Wall

.PHONY: all
all: server client

server: server.o common.o

client: client.o common.o

.PHONY: clean cleanobj
clean: cleanobj
	rm server client
cleanobj:
	rm *.o
