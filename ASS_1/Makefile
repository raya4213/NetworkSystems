CC=gcc
CFLAGS = -g 


all: client server

client: udp-send.o 
	$(CC) -o client udp-send.o $(LIBS)

server: udp-recv.o 
	$(CC) -o server udp-recv.o $(LIBS)

udp-send.o: udp-send.c

udp-recv.o: udp-recv.c

clean:
	rm -f udp-send udp-recv udp-send.o udp-recv.o 
