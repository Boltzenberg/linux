CC=g++

dnstree:  DNSDebug.cxx DNSDebug.h dnstree.cxx DNS.cxx DNS.h DNSResponse.cxx DNSResponse.h
	$(CC) -g -o $@ $^

.PHONY:  clean

clean:
	rm -rf dnstree
