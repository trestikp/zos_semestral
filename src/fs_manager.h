#ifndef _FS_MANAGER_H_
#define _FS_MANAGER_H_

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <sys/statvfs.h>
#include "file_system.h"
#include "general_functions.h"

int load_filesystem();
int format(char *size);
int mkdir(char *dir_name);
int ls(char *path);


#endif
