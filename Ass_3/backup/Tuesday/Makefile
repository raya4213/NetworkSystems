CC=gcc
CFLAGS = -g 


all: dfc dfs

dfc: dfc.o 
	$(CC) -o dfc dfc.o $(LIBS)

dfs: dfs.o 
	$(CC) -o dfs dfs.o $(LIBS)

dfc.o: dfc.c

dfs.o: dfs.c

clean:
	rm -f dfs dfc dfs.o dfc.o 
