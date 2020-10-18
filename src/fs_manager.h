#ifndef _FS_MANAGER_H_
#define _FS_MANAGER_H_

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <sys/statvfs.h>
#include "file_system.h"
#include "general_functions.h"

uint8_t load_filesystem();
uint8_t format(char *size);
uint8_t mkdir(char *dir_name);

#endif
