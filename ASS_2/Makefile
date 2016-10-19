CC=gcc
CFLAGS = -g 


all: webserver


webserver: main.o 
	$(CC) -o webserver main.o $(LIBS)

main.o: main.c

clean:
	rm -f webserver main.o 
