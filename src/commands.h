#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "file_system.h"
#include "general_functions.h"

#define SUPERBLOCK_CREATION_ERROR "Failed to initialize superblock"

int create_filesystem(uint64_t max_size);
int make_directory(char *name, int32_t parent_nid);
int list_dir_contents(int32_t node_id );
int traverse_path(char *path);
int change_dir(int32_t target_id);
int print_working_dir();

#endif
