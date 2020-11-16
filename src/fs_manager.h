#ifndef _FS_MANAGER_H
#define _FS_MANAGER_H

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <sys/statvfs.h>
#include "commands.h"
#include "general_functions.h"

int load_filesystem();
int format(char *size);
int mkdir(char *dir_name);
int ls(char *path);


#endif
