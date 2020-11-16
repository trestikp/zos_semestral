/**************************************/
/* 				      */
/*	Includes		      */
/*				      */
/**************************************/


#include "file_system.h"

/**************************************/
/* 				      */
/*	Global Variables	      */
/*				      */
/**************************************/


superblock *sblock = NULL;
inode *position = NULL;
const inode *root = NULL;
extern char *fs_filename;
extern FILE *fs_file;


/**************************************/
/* 				      */
/*	Support functions - general   */
/*				      */
/**************************************/


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

inode *load_inode_by_id(int32_t node_id) {
	inode *nd = malloc(sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);

	uint64_t address = sblock->inode_start_address +
			   (node_id - 1) * sizeof(inode);

	fseek(fs_file, address, SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);
	
	return nd;
}

/*
	Finds and allocates* free inode in bitmap
	returns: 0 - no inode is free
		address of inode in bits from bitmap start otherwise
*/
int32_t allocate_free_inode() {
	// returns address in bits from bitmapi address
	int32_t address = 0;
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

int32_t allocate_free_block() {
	int32_t block = 0;
	uint8_t byte = 0x00;
	int i = 0;

	fseek(fs_file, sblock->bitmap_start_address, SEEK_SET);
	
	while(block < sblock->cluster_count) {
		byte = fgetc(fs_file);
		for(i = (sizeof(uint8_t)) * 8 - 1; i >= 0; i--) {
			block++;

			if(!(byte & (1 << i))) {
				byte |= (1 << i);
				fseek(fs_file, -1, SEEK_CUR);
				fputc(byte, fs_file);

				return block;
			}
		}
	}

	return 0;
}


/**************************************/
/* 				      */
/*	Support functions -	      */
/*		command specific      */
/*				      */
/**************************************/


/*
	Returns number of bytes needed for bitmapi (inode bitmap)
*/
superblock *create_superblock(uint64_t max_size) {
	int ibmp_size = 0, bbmp_size = 0;

	superblock *sb = malloc(sizeof(superblock));
	return_error_on_condition(!sb, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);
	/*
	if(!sb) {
		//printf("Failed to create superblock. ERROR: Out of RAM");
		//create some free all function?
		return NULL;
	}
	*/

	uint64_t size = sizeof(superblock);

	// 6B bitmaps (+) -> 8 inodes + 40 blocks 
	int cycle_increment = 6 + (8 * sizeof(inode)) + (40 * BLOCK_SIZE);

	while((size + cycle_increment) <= max_size) {
		size += cycle_increment;
		sb->cluster_count += 40;
		ibmp_size++;
		bbmp_size += 5;
	}

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

void print_ls_record(inode *nd, char *name) {
	switch(nd->isDirectory) {
		case 0: printf("f\t"); break;
		case 1: printf("d\t"); break;
		case 2: printf("l\t"); break;
	}

	printf("%d\t%d\t%s\n", nd->file_size, nd->nodeid, name);
}

/**************************************/
/* 				      */
/*	Command functions	      */
/*				      */
/**************************************/

int search_dir(char *name, int32_t *from_nid) {
	uint64_t address = 0;
	int item_counter = 0;

	inode *nd = malloc(sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	directory_item *di = malloc(sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);


	address = sblock->inode_start_address + (*from_nid - 1) * sizeof(inode);

	fseek(fs_file, address, SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);

	if(nd->isDirectory != 1) {
		free(nd);
		free(di);
		return 2;
	}

	fseek(fs_file, sblock->data_start_address + nd->direct1 * sblock->cluster_size, SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	while(di->inode != 0 && item_counter < MAX_DIR_ITEMS_IN_BLOCK) {
		if(!strcmp(di->item_name, name)) {
			*from_nid = di->inode;
			break;
		} else {
			fread(di, sizeof(directory_item), 1, fs_file);
		}
	}

	//no dir item with name found
	if(!di->inode) {
		free(nd);
		free(di);
		 
		return 1;
	}
	
	free(nd);
	free(di);

	return 0;
}
