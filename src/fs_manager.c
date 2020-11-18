#include "fs_manager.h"
#include "file_system.h"


char *fs_filename = NULL;
FILE *fs_file = NULL;
extern superblock *sblock;
extern inode *position;
extern const inode *root;
static bool fs_loaded = false;

/*
	Actually only loads superblock to global variable *sblock
*/
int load_filesystem() {
	sblock = malloc(sizeof(superblock));
	return_error_on_condition(!sblock, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	position = malloc(sizeof(inode));
	return_error_on_condition(!position,
				  MEMORY_ALLOCATION_ERROR_MESSAGE, 1);


	fseek(fs_file, 0, SEEK_SET);

	if((fread(sblock, sizeof(superblock), 1, fs_file) != 1)) {
		print_error("Failed to read superblock from file!");
		return 1;
	}

	fseek(fs_file, sblock->inode_start_address, SEEK_SET);
	if((fread(position, sizeof(inode), 1, fs_file)) != 1) {
		print_error("Failed to read root inode");
		return 2;
	}

	// hack it ... - loading to const
	*(inode**) &root = position;

	fs_loaded = true;

	return 0;
}

int format(char *size) {
	//FILE *fs_file = NULL;
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

	create_filesystem(max_size);
	load_filesystem();
	//fclose(fs_file);
	free(vfs);

	return 0;
}

int extract_name_from_path(char *path, char **name) {
        int len = strlen(path), off = len;
        
        while(path[off - 1] != '/' && off > 0) { 
                off--;
        } 

        int diff = len - off;

        *name = calloc(sizeof(char), diff + 1);
        
        memcpy(*name, path + off, diff);
        name[diff + 1] = 0x00;
        
        bzero(&path[off], diff);
        
        return 0;
}	

int mkdir(char *path) {
	return_error_on_condition(!fs_file, FILE_OPEN_ERROR, 7);

	char *dir_name = calloc(sizeof(char), strlen(path) + 1);
	memcpy(dir_name, path, strlen(path));

	char *name_new = NULL;

	printf("full path: %s\n", dir_name);
	extract_name_from_path(dir_name, &name_new);
	printf("searching for %s in %s\n", name_new, dir_name);

	if(dir_name == NULL) printf("DIR NAME IS NUILL");
	printf("DIR LEN: %ld\n", strlen(dir_name));
	
	int parent_id = traverse_path(dir_name);
	if(!parent_id) return 1;
	
	printf("making dir at: %d\n", parent_id);
	free(dir_name);
	
	//make_directory(dir_name, parent_it);

	return 0;
}

int ls(char *path) {
	int32_t end_nid = 0;
	end_nid = traverse_path(path);

	if(!end_nid) {
		return 0;
	}

	list_dir_contents(end_nid);
	
	return 0;
}
