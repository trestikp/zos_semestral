#include "console.h"
#include <stdio.h>

uint8_t run(char *file_name) {
	extern FILE *fs_file;
	extern char *fs_filename;

	printf("Welcome to ZOS semestral shell.\n");

	if((fs_file = fopen(file_name, "r"))) {
		if(freopen(NULL, "rb+", fs_file)) {
			load_filesystem();
		} else {
			printf("Failed to reopen file before loading filesytem. Aborting program...\n");
 			return 1;
		}
	} else {
		printf("INFO: File does not exist. Run \"format\" to initialize new filesystem in %s\n", file_name);
	}
		
	fs_filename = file_name;

	run_console();

	return 0;
}

int main(int argc, char *argv[]) {
	if(argc < 2 || argc > 2) {
		printf("Expecting file_name argument. Please rerun the program this way: runfs file_name\n");
 		return 1;
	}

	run(argv[1]);

	return 0;
}
