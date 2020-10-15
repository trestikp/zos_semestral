#include "file_system.h"
#include "general_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

superblock *sblock = NULL;

//TODO: REMOVE LATER
void print_superblock(superblock *sb) {
	printf("size of supeblock: %ld\n", sizeof(superblock));
	printf("size of inode: %ld\n", sizeof(inode));
	printf("--------\n");
	printf("ibmp address: %d\n", sb->bitmapi_start_address);
	printf("bbmp address: %d\n", sb->bitmap_start_address);
	printf("inodes address: %d\n", sb->inode_start_address);
	printf("data address: %d\n", sb->data_start_address);
	printf("--------\n");
	printf("block count: %d\n", sb->cluster_count);
	printf("--------\n");
}

// na 1 inode 5 bloku
// 1B inodu -> 5B blocku = 6B celkove

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
	sb->bitmapi_start_address = sizeof(superblock) + 1;
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

uint8_t create_filesystem(FILE *f, uint64_t max_size) {
	//165432 - exact size for 4B ibmp, 20B bbmp
	//173625 - 4B ibmp, 21B bbmp
	//206718 - exact size for 5B ibmp, 25B bbmp
	//superblock *sb = create_superblock(165432);
	//superblock *sb = create_superblock(173625);
	//superblock *sb = create_superblock(206718);
	//superblock *sb = create_superblock(206717);
	//superblock *sb = create_superblock(206719);
	//superblock *sb = create_superblock(206396);
	//170553 - 4B ibmp, 21B bbmp - 5 blocks 
	//superblock *sb = create_superblock(170553);
	//superblock *sb = create_superblock(170552);
	//superblock *sb = create_superblock(max_size);
	sblock = create_superblock(max_size);
	if(!sblock) {
		//TODO: memory freeing, program exit error code
		return 1;
	}
	print_superblock(sblock);

	fseek(f, 0, SEEK_SET);
	printf("position in file is: %ld\n", ftell(f));
	fwrite(sblock, sizeof(superblock), 1, f);

	return 0;
}
