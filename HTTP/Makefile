CFLAGS+=-g -Wall -O2 -D_FORTIFY_SOURCE=1
LDFLAGS+=-g -Wall -O2 -D_FORTIFY_SOURCE=1

.PHONY: all
all: proxy client test_list test_cache

proxy: proxy.o common.o strlcpy.o list.o cache.o

client: client.o common.o strlcpy.o

test_list: test_list.o list.o strlcpy.o common.o

test_cache: test_cache.o cache.o strlcpy.o common.o

.PHONY: clean cleanobj
clean: cleanobj
	rm proxy client test_list test_cache
cleanobj:
	rm *.o
