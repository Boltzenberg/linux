CC=gcc
AR=ar

ctest1.o: ctest1.c
	$(CC) -Wall -c $^

ctest.a: ctest1.o
	$(AR) -cvq $@ $^

ctest: prog.c ctest.a
	$(CC) -o $@ $^

.PHONY:  clean

clean:
	rm ctest1.o ctest.a ctest
