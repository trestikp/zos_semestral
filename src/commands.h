#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "file_system.h"
#include "general_functions.h"

#define SUPERBLOCK_CREATION_ERROR "Failed to initialize superblock"

int create_filesystem(uint64_t max_size);
int make_directory(char *name, int32_t parent_nid);
int list_dir_contents(int32_t node_id);
int traverse_path(char *path);
int change_dir(int32_t target_id);
int print_working_dir();
int remove_directory(int32_t node_id);
int in_copy(char *source, int32_t t_node, char *source_name, char *target_name);
int cat_file(int32_t where, char *name);
int remove_file(int32_t where, char *name);
int move(int32_t sparent, int32_t tparent, char* sname, char *tname);
int copy(int32_t sparent, int32_t tparent, char* sname, char *tname);
int node_info(int32_t where, char *name);
int out_copy(int32_t where, char *name, char *target);
int symbolic_link(int32_t src, int32_t par, char *name);

#endif
