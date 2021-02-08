#include "fs_manager.h"
#include "commands.h"
#include "file_system.h"
#include "general_functions.h"


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
		print_error("Unrecognized unit format. Example unit format: MB, GB");
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
	
	//printf("making dir at: %d\n", parent_id);
	//if(does_item_exist_in_dir(name_new, parent_id)) return 1;
	if(search_dir(name_new, &parent_id) != 1) return 1;
	
	make_directory(name_new, parent_id);

	free(dir_name);

	return 0;
}

int rmdir(char* path) {
	return_error_on_condition(!fs_file, FILE_OPEN_ERROR, 7);
	
	int32_t target = traverse_path(path);

	if(!target) return 1;

	remove_directory(target);

	return 0;
}

int cd(char *path) {
	int32_t target = traverse_path(path);
	
	printf("cd target: %d\n", target);

	change_dir(target);

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

int pwd() {
	print_working_dir();

	return 0;
}


int incp(char *source, char *target) {
	char *source_name = NULL, *target_name = NULL;
	char *source_cpy = calloc(strlen(source) + 1, sizeof(char));
	char *target_cpy = calloc(strlen(target) + 1, sizeof(char));

	strcpy(source_cpy, source);
	strcpy(target_cpy, target);
	extract(source_cpy, &source_name);
	extract(target_cpy, &target_name);

	int32_t tnode = traverse_path(target_cpy);
	if(!tnode) return 1;

	in_copy(source, tnode, source_name, target_name);

	free(source_cpy);
	free(target_cpy);

	return 0;
}

int cat(char *path) {
	char *name = NULL;
	char *path_cpy = calloc(strlen(path) + 1, sizeof(char));

	strcpy(path_cpy, path);
	extract(path_cpy, &name);

	int32_t tnode = traverse_path(path_cpy);
	if(!tnode) return 1;

	cat_file(tnode, name);

	free(path_cpy);

	return 0;
}

int rm(char *path) {
	char *name = NULL;
	char *path_cpy = calloc(strlen(path) + 1, sizeof(char));

	strcpy(path_cpy, path);
	extract(path_cpy, &name);

	int32_t tnode = traverse_path(path_cpy);
	if(!tnode) return 1;

	remove_file(tnode, name);

	return 0;
}


int mv(char *source, char *target) {
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

	move(snode, tnode, source_name, target_name);

	free(source_cpy);
	free(target_cpy);

	return 0;
}


int cp(char *source, char *target) {
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

	copy(snode, tnode, source_name, target_name);

	free(source_cpy);
	free(target_cpy);
	
	return 0;
}


int info(char *path) {
	char *name = NULL;
	char *path_cpy = calloc(strlen(path) + 1, sizeof(char));

	strcpy(path_cpy, path);
	extract(path_cpy, &name);

	int32_t tnode = traverse_path(path_cpy);
	if(!tnode) return 1;

	node_info(tnode, name);

	return 0;
}


int outcp(char *source, char *target) {
	char *name = NULL;
	char *source_cpy = calloc(strlen(source) + 1, sizeof(char));

	strcpy(source_cpy, source);
	extract(source_cpy, &name);

	int32_t tnode = traverse_path(source_cpy);
	if(!tnode) return 1;

	out_copy(tnode, name, target);
	
	return 0;
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

	symbolic_link(src, par, name);

	return 0;
}
