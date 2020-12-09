CFLAGS=-std=c11 -g -static

ccb: ccb.c

test: ccb
	./test.sh

clean:
	rm -f ccb *.o *~ tmp*

.PHONY: test clean