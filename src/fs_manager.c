#include "fs_manager.h"
#include "commands.h"
#include "file_system.h"
#include "general_functions.h"


char *fs_filename = NULL;
FILE *fs_file = NULL;

extern superblock *sblock;
extern inode *position;
extern const inode *root;

bool fs_loaded = false;

/*
	Actually only loads superblock to global variable *sblock
*/
int load_filesystem() {
	fs_loaded = false;

	sblock = malloc(sizeof(superblock));
	return_error_on_condition(!sblock, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	position = malloc(sizeof(inode));
	return_error_on_condition(!position,
				  MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	root = calloc(sizeof(inode), 1);
	return_error_on_condition(!root, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);


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
	//*(inode**) &root = position;
	memcpy(*(inode**) &root, position, 1);

	fs_loaded = true;

	return 0;
}


int format(char *size) {
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
		print_error("Unrecognized unit format. Example unit format: MB, GB");
 		return 1;
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
				return 2; 
		}

		if(units[1] != 'B') {
			print_error("Unrecognized unit.");
			return 3;
		}

		// allow "bits" ??? 
		/* nope I don't want to deal with bits, its stupid
		if(units[1] == 'b') max_size /= 8;
		else if(units[1] != 'B') {
			print_error("Unrecognized unit.");
			return 3;
		}
		*/
	}

	// if there isnt enough space on storage device for unprivileged user
	if(max_size > (vfs->f_bavail * vfs->f_bsize)) {
		free(vfs);
		print_command_result(FORMAT_CMD_ERROR);
		return 4;
	}

	//return_error_on_condition(!fs_file, FILE_OPEN_ERROR, 7);
	if(!(fs_file = fopen(fs_filename, "wb+"))) {
		free(vfs);
		print_error("ERROR: Failed to open file");
		return 5;
	}

	create_filesystem(max_size);
	load_filesystem();
	free(vfs);

	return 0;
}


static inline char *pointer_offset(char *pointer, int offset) {
     return pointer + (offset * sizeof(char));
}

int extract(char *ppath, char **pname) {
     int len = strlen(ppath), off = len;

     if(*pointer_offset(ppath, (len - 1)) == '/') {
             *pointer_offset(ppath, (len - 1)) = 0x00;
             len--;
     }

     while(*pointer_offset(ppath, (off - 1)) != '/' && off > 0) {
             off--;
     }

     int diff = len - off;

     *pname = calloc(sizeof(char), diff + 1);

     memcpy(*pname, pointer_offset(ppath, off), diff);
     *pointer_offset(*pname, diff + 1) = 0x00;

     bzero(pointer_offset(ppath, off), diff);

     return 0;
}


int ugly_workaround_existence(char *path, int32_t target) {
	//ugly workaround to prevent listing even when the item @name doesn't exist
	//otherwise ls lists content of current dir, even when the final target doesn't exist
	if(path) {
		char *name = NULL;
		char *cpy = calloc(strlen(path) + 1, sizeof(char));
		strcpy(cpy, path);
		extract(cpy, &name);
	
		int32_t par_id = traverse_path(cpy);
		if(!par_id || (par_id == target)) return 1;
	}
	// end

	return 0;
}



int mkdir(char *path) {
	return_error_on_condition(!fs_file, FILE_OPEN_ERROR, 7);

	char *dir_name = calloc(strlen(path) + 1, sizeof(char));
	memcpy(dir_name, path, strlen(path));

	char *name_new = NULL;

	extract(dir_name, &name_new);

	//if(dir_name == NULL) printf("DIR NAME IS NUILL");
	//printf("DIR LEN: %ld\n", strlen(dir_name));
	
	int parent_id = traverse_path(dir_name);
	if(!parent_id) return 1;
	
	if(search_dir(name_new, &parent_id) != 1) {
		printf("EXISTS\n");
		return 1;
	}
	
	make_directory(name_new, parent_id);

	free(dir_name);

	return 0;
}

int rmdir(char* path) {
	return_error_on_condition(!fs_file, FILE_OPEN_ERROR, 7);
	
	int32_t target = traverse_path(path);

	if(!target) return 1;

	return remove_directory(target);

	//return 0;
}

int cd(char *path) {
	int32_t target = traverse_path(path);

	if(!target) return 1;

	if(ugly_workaround_existence(path, target)) return 2;

	return change_dir(target);
	//return 0;
}

int ls(char *path) {
	int32_t end_nid = 0;
	end_nid = traverse_path(path);

	if(!end_nid) return 1;

	if(ugly_workaround_existence(path, end_nid)) return 2;

	return list_dir_contents(end_nid);
	//return 0;
}

int pwd() {
	return print_working_dir();
	//return 0;
}


int incp(char *source, char *target) {
	int rv = 0;
	char *source_name = NULL, *target_name = NULL;
	char *source_cpy = calloc(strlen(source) + 1, sizeof(char));
	char *target_cpy = calloc(strlen(target) + 1, sizeof(char));

	strcpy(source_cpy, source);
	strcpy(target_cpy, target);
	extract(source_cpy, &source_name);
	extract(target_cpy, &target_name);

	int32_t tnode = traverse_path(target_cpy);
	if(!tnode) return 1;

	rv = in_copy(source, tnode, source_name, target_name);

	free(source_cpy);
	free(target_cpy);

	return rv;
}

int cat(char *path) {
	int rv = 0;
	char *name = NULL;
	char *path_cpy = calloc(strlen(path) + 1, sizeof(char));

	strcpy(path_cpy, path);
	extract(path_cpy, &name);

	int32_t tnode = traverse_path(path_cpy);
	if(!tnode) return 1;

	rv = cat_file(tnode, name);

	free(path_cpy);

	return rv;
}

int rm(char *path) {
	int rv = 0;
	char *name = NULL;
	char *path_cpy = calloc(strlen(path) + 1, sizeof(char));

	strcpy(path_cpy, path);
	extract(path_cpy, &name);

	int32_t tnode = traverse_path(path_cpy);
	if(!tnode) return 1;

	rv = remove_file(tnode, name);

	return rv; 
}


int mv(char *source, char *target) {
	int rv = 0;
	char *source_name = NULL, *target_name = NULL;
	char *source_cpy = calloc(strlen(source) + 1, sizeof(char));
	char *target_cpy = calloc(strlen(target) + 1, sizeof(char));

	strcpy(source_cpy, source);
	strcpy(target_cpy, target);
	extract(source_cpy, &source_name);
	extract(target_cpy, &target_name);

	//int32_t tnode = traverse_path(target);
	int32_t snode = traverse_path(source_cpy);
	if(!snode) return 1;
	int32_t tnode = traverse_path(target_cpy);
	if(!tnode) return 1;

	rv = move(snode, tnode, source_name, target_name);

	free(source_cpy);
	free(target_cpy);

	return rv; 
}


int cp(char *source, char *target) {
	int rv = 0;
	char *source_name = NULL, *target_name = NULL;
	char *source_cpy = calloc(strlen(source) + 1, sizeof(char));
	char *target_cpy = calloc(strlen(target) + 1, sizeof(char));

	strcpy(source_cpy, source);
	strcpy(target_cpy, target);
	extract(source_cpy, &source_name);
	extract(target_cpy, &target_name);

	int32_t snode = traverse_path(source_cpy);
	if(!snode) return 1;
	int32_t tnode = traverse_path(target_cpy);
	if(!tnode) return 1;

	rv = copy(snode, tnode, source_name, target_name);

	free(source_cpy);
	free(target_cpy);
	
	return rv; 
}


int info(char *path) {
	char *name = NULL;
	char *path_cpy = calloc(strlen(path) + 1, sizeof(char));

	strcpy(path_cpy, path);
	extract(path_cpy, &name);

	int32_t tnode = traverse_path(path_cpy);
	if(!tnode) return 1;

	return node_info(tnode, name);
	//return 0;
}


int outcp(char *source, char *target) {
	char *name = NULL;
	char *source_cpy = calloc(strlen(source) + 1, sizeof(char));

	strcpy(source_cpy, source);
	extract(source_cpy, &name);

	int32_t tnode = traverse_path(source_cpy);
	if(!tnode) return 1;

	return out_copy(tnode, name, target);
	//return 0;
}


int slink(char *source, char *link) {
	char *name = NULL;
	char *link_cpy = calloc(strlen(link) + 1, sizeof(char));

	strcpy(link_cpy, link);
	extract(link_cpy, &name);

	int32_t par = traverse_path(link_cpy);
	if(!par) return 1;
	int32_t src = traverse_path(source);
	if(!src) return 1;

	return symbolic_link(src, par, name);
	//return 0;
}
