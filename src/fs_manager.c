#include "fs_manager.h"
#include <stdio.h>
#include "file_system.h"
#include "general_functions.h"

//filesystem pathname?
FILE *fs_file = NULL;
extern superblock *sblock;

uint8_t load_filesystem() {
	//TODO: load FS (from file_system.c)

	//printf("super SHIT: %d\n", sblock->disk_size);

	return 0;
}

uint8_t format(char *size) {
	uint64_t max_size = 0;

	if(!(fs_file = freopen(NULL, "wb+", fs_file))) {
		print_error(FILE_REOPEN_ERROR_MESSAGE);
		return 7; // return some error number TODO
	}

	//TODO: parse size to size here (max_size)

	printf("ahoj");
	print_error_on_condition(fs_file == NULL, "fuck something blew");

	create_filesystem(fs_file, 170553);

	return 0;
}
