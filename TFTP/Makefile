CFLAGS+=-g -Wall
LDFLAGS+=-g -Wall

.PHONY: all
all: server test_list

server: server.o common.o list.o

test_list: test_list.o list.o

.PHONY: clean cleanobj
clean: cleanobj
	rm server test_list
cleanobj:
	rm *.o

.PHONY: test
test:
	./test_multiple_clients.sh
