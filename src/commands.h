#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "file_system.h"

int create_filesystem(uint64_t max_size);
int make_directory(char *name, int32_t parent_nid);
int list_dir_contents(int32_t node_id );

#endif
