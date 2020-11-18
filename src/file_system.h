#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#include "general_functions.h"
//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
//#include <sys/types.h>



#define ID_ITEM_FREE 0
#define BLOCK_SIZE 1024
#define MAX_ITEM_NAME_LENGTH 12
#define MAX_DIR_ITEMS_IN_BLOCK (int) (BLOCK_SIZE / sizeof(directory_item))

#define DEFAULT_SIGNATURE "trestikp"
#define DEFAULT_DESCRIPTION "This is a semestral work for KIV/ZOS.\
 Objective of this project is to create pseudo inode filesystem."

typedef struct super_block {
    char signature[9];              //login autora FS
    char volume_descriptor[251];    //popis vygenerovaného FS
    int32_t disk_size;              //celkova velikost VFS
    int32_t cluster_size;           //velikost clusteru = block size
    int32_t cluster_count;          //pocet clusteru = block count
    int32_t bitmapi_start_address;  //adresa pocatku bitmapy i-uzlů
    int32_t bitmap_start_address;   //adresa pocatku bitmapy datových bloků
    int32_t inode_start_address;    //adresa pocatku  i-uzlů
    int32_t data_start_address;     //adresa pocatku datovych bloku  
} superblock;


typedef struct pseudo_inode {
    int32_t nodeid;                 //ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
    int8_t isDirectory;             //0 = soubor, 1 = adresar, 2 = s. link
    int8_t references;              //počet odkazů na i-uzel, používá se pro hardlinky
    int32_t file_size;              //velikost souboru v bytech
    int32_t direct1;                // 1. přímý odkaz na datové bloky
    int32_t direct2;                // 2. přímý odkaz na datové bloky
    int32_t direct3;                // 3. přímý odkaz na datové bloky
    int32_t direct4;                // 4. přímý odkaz na datové bloky
    int32_t direct5;                // 5. přímý odkaz na datové bloky
    int32_t indirect1;              // 1. nepřímý odkaz (odkaz - datové bloky)
    int32_t indirect2;              // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
} inode;


typedef struct directory__item {
    int32_t inode;                   // inode odpovídající souboru
    char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak
} directory_item;

/*
superblock *sblock = NULL;
inode *position = NULL;
const inode *root = NULL;
extern char *fs_filename;
extern FILE *fs_file;
*/


inode *load_inode_by_id(int32_t node_id);
int32_t allocate_free_inode();
int32_t allocate_free_block();

//int create_filesystem(uint64_t max_size);
//int make_directory(char *name, int32_t parent_nid);
//int list_dir_contents(int32_t node_id );
int search_dir(char *name, int32_t *from_nid);
int append_dir_item(directory_item *di, inode *node);

#endif
