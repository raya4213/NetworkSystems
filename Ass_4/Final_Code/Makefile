CC=gcc
CFLAGS = -g 


all: webproxy


webproxy: webproxy.o 
	$(CC) -o webproxy webproxy.o -lcrypto

webproxy.o: webproxy.c

clean:
	rm -f webproxy webproxy.o 
