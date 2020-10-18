CC = gcc
PARAMS = -Wall -g

all: clean run remove_o

clean:
	rm -f runfs

console:
	${CC} ${PARAMS} -c src/console.c

file_sytem:
	${CC} ${PARAMS} -c src/file_system.c

fs_manager:
	${CC} ${PARAMS} -c -lm src/fs_manager.c

run: console file_sytem fs_manager
	${CC} ${PARAMS} -o runfs src/main.c console.o file_system.o fs_manager.o

remove_o:
	rm -f *.o
