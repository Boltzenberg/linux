CC=gcc
CPP=g++
COPTS=-g

client:  client.c
	$(CC) $(COPTS) -o $@ $^

server:  server.c
	$(CC) $(COPTS) -o $@ $^

.PHONY:  clean

clean:
	rm -rf client server