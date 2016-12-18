CC=gcc
CPP=g++
COPTS=-g

main:  main.c
	$(CC) $(COPTS) -o $@ $^

sock:  sock.c
	$(CC) $(COPTS) -o $@ $^

hello:  hello.cxx
	$(CPP) $(COPTS) -o $@ $^
	
dns:  dns.c
	$(CC) $(COPTS) -o $@ $^

.PHONY:  clean

clean:
	rm -rf main sock hello dns
