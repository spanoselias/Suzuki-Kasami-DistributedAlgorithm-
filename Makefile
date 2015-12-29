CC=gcc

commMake:  node.c  
	$(CC) node.c -lm -lpthread -o node 
	 
	 

all: run

run: cli
	./node -p 20000
 

.PHONY: all run

clean:
	rm -f *.o  

