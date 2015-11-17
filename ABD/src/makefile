CC=gcc

commMake: client.c
	$(CC) client.c  -o cli

all: run

run: cli
	./icli  

.PHONY: all run

clean:
	rm -f *.o


