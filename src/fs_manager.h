#ifndef _FS_MANAGER_H
#define _FS_MANAGER_H

/**************************************/
/* 				      */
/*	Includes		      */
/*				      */
/**************************************/


#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <sys/statvfs.h>
#include "commands.h"

/**************************************/
/* 				      */
/*	Function prototypes	      */
/*				      */
/**************************************/


int load_filesystem();
int format(char *size);
int mkdir(char *path);
int cd(char *path);
int ls(char *path);
int pwd();
int incp(char *source, char *target);
int rmdir(char *path);
int cat(char *path);
int rm(char *path);
int mv(char *source, char *target);
int cp(char *source, char *target);
int info(char *path);
int outcp(char *source, char *target);
int slink(char *source, char *link);

#endif
