#include "file_system.h"
#include "general_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

superblock *sblock = NULL;
inode *position = NULL;
extern char *fs_filename;
extern FILE *fs_file;

//TODO: REMOVE LATER
void print_superblock(superblock *sb) {
	printf("--------\n");
	printf("size of supeblock: %ld\n", sizeof(superblock));
	printf("size of inode: %ld\n\n\n", sizeof(inode));
	printf("ibmp address: %d\n", sb->bitmapi_start_address);
	printf("bbmp address: %d\n", sb->bitmap_start_address);
	printf("inodes address: %d\n", sb->inode_start_address);
	printf("data address: %d\n\n", sb->data_start_address);
	printf("block count: %d\n", sb->cluster_count);
	printf("--------\n");
}

inode *load_inode_from_address(int32_t address) {
	inode *nd = malloc(sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);

	fseek(fs_file, address, SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);
	
	return nd;
}

/*
	Finds and allocates* free inode in bitmap
	returns: 0 - no inode is free
		address of inode in bits from bitmap start otherwise
*/
uint64_t allocate_free_inode() {
	// returns address in bits from bitmap address
	uint64_t address = 0;
	uint8_t byte = 0x00;
	int i = 0;


	fseek(fs_file, sblock->bitmapi_start_address, SEEK_SET);
	while(address <= ((sblock->bitmap_start_address - 
		  	   sblock->bitmapi_start_address) * 8)) {
		byte = fgetc(fs_file);
		for(i = (sizeof(uint8_t) * 8 - 1); i >= 0; i--) {
			address++;

			if(!(byte & (1 << i))) {
				byte |= (1 << i);
				fseek(fs_file, -1, SEEK_CUR);
				fputc(byte, fs_file);

				return address;
			}
		}
		//address += 8;
	}

	return 0;
}

/*
	Returns number of bytes needed for bitmapi (inode bitmap)
*/
superblock *create_superblock(uint64_t max_size) {
	int ibmp_size = 0;
	int bbmp_size = 0;

	superblock *sb = malloc(sizeof(superblock));
	if(!sb) {
		//printf("Failed to create superblock. ERROR: Out of RAM");
		//create some free all function?
		return NULL;
	}
	uint64_t size = sizeof(superblock);

	// 6B bitmaps (+) -> 8 inodes + 40 blocks 
	int cycle_increment = 6 + (8 * sizeof(inode)) + (40 * BLOCK_SIZE);

	while((size + cycle_increment) <= max_size) {
		size += cycle_increment;
		sb->cluster_count += 40;
		ibmp_size++;
		bbmp_size += 5;
	}

	/*
	cycle_increment = 1 + (8 * BLOCK_SIZE);
	while((size + cycle_increment) <= max_size) {
		size += cycle_increment;
		sb->cluster_count += 8;
		bbmp_size += 1;
	}
	*/

	printf("size before block fill: %ld\n", size);

	while((size + BLOCK_SIZE + 1) <= max_size) {
		//printf("b\n");
		bbmp_size++;
		size++;
		do{
			size += BLOCK_SIZE;
			sb->cluster_count++;
		} while((size + BLOCK_SIZE) <= max_size && sb->cluster_count % 8);
	}
	
	sb->disk_size = size;
	sb->cluster_size = BLOCK_SIZE;
	//sb->bitmapi_start_address = sizeof(superblock) + 1; //dunno about + 1 file index from 0?
	sb->bitmapi_start_address = sizeof(superblock);
	sb->bitmap_start_address = sb->bitmapi_start_address + ibmp_size;
	sb->inode_start_address = sb->bitmapi_start_address + ibmp_size +
				  bbmp_size;
	sb->data_start_address = sb->bitmapi_start_address + ibmp_size +
				 bbmp_size + (sizeof(inode) * (ibmp_size * 8));
	sprintf(sb->signature, DEFAULT_SIGNATURE);
	sprintf(sb->volume_descriptor, DEFAULT_DESCRIPTION);

	printf("max size: %ld\n", max_size);
	printf("size: %ld\n", size);
	printf("size left: %ld\n\n", max_size - size);

	return sb;
}

int create_filesystem(uint64_t max_size) {
	int i = 0;
	inode *node = NULL;
	directory_item *ditem = NULL;

	sblock = create_superblock(max_size);
	if(!sblock) {
		//TODO: memory freeing?, program exit error code
		return 1;
	}
	print_superblock(sblock);

	fseek(fs_file, 0, SEEK_SET);
	fwrite(sblock, sizeof(superblock), 1, fs_file);

	printf("IBMP WRITE: %ld\n", ftell(fs_file));
	//first inode bmp byte (root)
	fputc(0x80, fs_file);
	//fwrite? 
	for(i = 0; i < (sblock->bitmap_start_address -
	sblock->bitmapi_start_address - 1); i++) {
		fputc(0x00, fs_file);
	}

	printf("BBMP WRITE: %ld\n", ftell(fs_file));
	//first block bmp byte(root)
	fputc(0x80, fs_file);
	for(i = 0; i < (sblock->inode_start_address -
	sblock->bitmap_start_address - 1); i++) {
		fputc(0x00, fs_file);
	}

	//first inode
	node = malloc(sizeof(inode));
	node->nodeid = 1;
	node->isDirectory = true;
	node->references = 1;
	node->direct1 = sblock->data_start_address;

	printf("INODE WRITE: %ld\n", ftell(fs_file));
	fwrite(node, sizeof(inode), 1, fs_file);

	node->references = 0;
	node->isDirectory = false;
	node->direct1 = 0;

	for(i = 1; i < ((sblock->bitmap_start_address -
	sblock->bitmapi_start_address) * 8); i++) {
		node->nodeid = i + 1;
		fwrite(node, sizeof(inode), 1, fs_file);
	}

	free(node);
	//the rest of inodes
	//node->nodeid = ID_ITEM_FREE;
	//reset other variables?
	/*
	fwrite(node, sizeof(inode), ((sblock->bitmap_start_address -
		sblock->bitmapi_start_address) * 8), fs_file);
	*/
	//fwrite refuses to work correctly with this..
	/*
	for(i = 0; i < ((sblock->bitmap_start_address -
	sblock->bitmapi_start_address) * 8 * sizeof(inode) -
	sizeof(inode)); i++) {
		fputc(0x00, fs_file);
	}
	*/


	//first block
	ditem = malloc(sizeof(directory_item));
	ditem->inode = 1;
	bzero(ditem->item_name, MAX_ITEM_NAME_LENGTH);
	ditem->item_name[0] = '/';

	printf("BLOCK WRITE: %ld\n", ftell(fs_file));
	fwrite(ditem, sizeof(directory_item), 1, fs_file);

	memcpy(ditem->item_name, "..", 1);
	fwrite(ditem, sizeof(directory_item), 1, fs_file);

	memcpy(ditem->item_name, "..", 2);
	fwrite(ditem, sizeof(directory_item), 1, fs_file);

	for(i = 0; i < (sblock->cluster_size *
	sblock->cluster_count - sizeof(directory_item) * 3); i++) {
		fputc(0x00, fs_file);
	}
	printf("BLOCK END: %ld\n", ftell(fs_file));

	free(ditem);

	return 0;
}

int make_directory(char *name) {
	uint64_t pos = 0;

	inode *new = malloc(sizeof(inode));
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);

	pos = allocate_free_inode();
	fseek(fs_file, (sblock->inode_start_address + pos * sizeof(inode)), SEEK_SET);
	fread(new, sizeof(inode), 1, fs_file);

	printf("pos: %ld\nid: %d\n", pos, new->nodeid);

	return 0;
}

int list_dir_contents(inode *target) {

	return 0;
}
