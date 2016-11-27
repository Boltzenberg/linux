CC=gcc
CPP=g++

main:  main.c
	$(CC) -o $@ $^

sock:  sock.c
	$(CC) -o $@ $^

hello:  hello.cxx
	$(CPP) -o $@ $^
	
dns:  dns.c
	$(CC) -o $@ $^

.PHONY:  clean

clean:
	rm -rf main sock hello dns
