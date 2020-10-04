CC = gcc
PARAMS = -Wall -g

all: clean run remove_o

clean:
	rm -f runfs

console:
	${CC} ${PARAMS} -c src/console.c

run: console
	${CC} ${PARAMS} -o runfs src/main.c console.o

remove_o:
	rm -f *.o
