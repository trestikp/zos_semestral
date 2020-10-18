#include "fs_manager.h"
#include "general_functions.h"
#include <math.h>
#include <stdio.h>


char *fs_filename = NULL;
extern superblock *sblock;
static bool fs_loaded = false;

/*
	Actually only loads superblock to global variable *sblock
*/
uint8_t load_filesystem() {
	FILE *fs_file = fopen(fs_filename, "r");

	fseek(fs_file, 0, SEEK_SET);

	if((fread(sblock, sizeof(superblock), 1, fs_file) != 1)) {
		print_error("Failed to read superblock from file!");
		return 1;
	}

	fs_loaded = true;
	fclose(fs_file);

	return 0;
}

uint8_t format(char *size) {
	FILE *fs_file = NULL;
	uint64_t max_size = 0;
	int number = 0;
	char units[3] = {0};
	struct statvfs *vfs = malloc(sizeof(struct statvfs));

	// quickfix to prevent "0" size with non-existing files
	if((fs_file = fopen(fs_filename, "r"))) {
		statvfs(fs_filename, vfs);
		fclose(fs_file);
	} else {
		statvfs(".", vfs);
	}

	sscanf(size, "%d%3c", &number, units);

	// if unit format is more then 2 characters
	if(units[2]) {
		print_error("Unrecognized unit format.\
 Example unit format: MB, GB");
 		return 7; // TODO return error value
	} else { 
		switch(units[0]) {
			case 'k':
			case 'K': max_size = number * pow(2, 10); break;
			case 'm':
			case 'M': max_size = number * pow(2, 20); break;
			case 'g':
			case 'G': max_size = number * pow(2, 30); break;
			case 't':
			case 'T': max_size = number * pow(2, 40); break;
			case 'p':
			case 'P': max_size = number * pow(2, 50); break;
			default: print_error("Unrecognized size.");
				return 7; // TODO error code
		}

		// allow "bits" ??? 
		if(units[1] == 'b') max_size /= 8;
		else if(units[1] != 'B') {
			print_error("Unrecognized unit.");
			return 7; // TODO err code
		}
	}

	// if there isnt enough space on storage device for unprivileged user
	if(max_size > (vfs->f_bavail * vfs->f_bsize)) {
		free(vfs);
		print_command_result(FORMAT_CMD_ERROR);
		return 7; // TODO
	}

	//return_error_on_condition(!fs_file, FILE_OPEN_ERROR, 7);
	if(!(fs_file = fopen(fs_filename, "wb+"))) {
		free(vfs);
		print_error("File open failed.. TODO"); // TODO error msg
		return 7; // return some error number TODO
	}

	create_filesystem(fs_file, max_size);
	fclose(fs_file);
	free(vfs);

	return 0;
}

uint8_t mkdir(char *dir_name) {
	FILE *fs_file = fopen(fs_filename, "ab+");

	return_error_on_condition(!fs_file, FILE_OPEN_ERROR, 7);

	make_directory(fs_file, dir_name);

	return 0;
}
