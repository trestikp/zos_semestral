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
/*	temp			      */
/*				      */
/**************************************/

void print_inode_bitmap() {
	int32_t address = 0;
	uint8_t byte = 0x00;
	int i = 0;

	fseek(fs_file, sblock->bitmapi_start_address, SEEK_SET);

	printf("Inode bitmap: ");
	while(address <= ((sblock->bitmap_start_address - sblock->bitmapi_start_address) * 8)) {
		byte = fgetc(fs_file);

		for(i = 7; i >=0; i--) {
			if(byte & (1 << i)) {
				printf("1");
			} else {
				printf("0");
			}
		}

		printf(" ");
	}
	printf("\n");
}


/**************************************/
/* 				      */
/*	Support functions - general   */
/*				      */
/**************************************/

/**
	Calculates the byte address in file of a block. 
	Blocks are indexes beginning with 0
*/
uint64_t get_block_address_from_position(int32_t position) {
	return sblock->data_start_address + (position * sblock->cluster_size);
}

/**
	Calculates the byte address in file of a inode.
	Calculates by ID which is "position + 1" -> to get the position is ID - 1
*/
uint64_t get_inode_address_from_id(int32_t node_id) {
	return sblock->inode_start_address + ((node_id - 1) * sizeof(inode));
}

/**
	Loads inode on position ID - 1:
	ID is position + 1 -> first inode has ID 1
*/
inode *load_inode_by_id(int32_t node_id) {
	inode *nd = calloc(1, sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);

	fseek(fs_file, get_inode_address_from_id(node_id), SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);
	
	return nd;
}


/*
	Finds and allocates* free inode in bitmap
	returns: 0 - no inode is free
		 address - position of inode (indexing from 0)
*/
int32_t allocate_free_inode() {
	// returns address in bits from bitmapi address
	int32_t address = 0;
	uint8_t byte = 0x00;
	int i = 0;


	fseek(fs_file, sblock->bitmapi_start_address, SEEK_SET);
	while(address <= ((sblock->bitmap_start_address - sblock->bitmapi_start_address) * 8)) {
		byte = fgetc(fs_file);
		for(i = (sizeof(uint8_t) * 8 - 1); i >= 0; i--) {
			if(!(byte & (1 << i))) {
				byte |= (1 << i);
				fseek(fs_file, -1, SEEK_CUR);
				fputc(byte, fs_file);

				return address;
			}

			address++;
		}
	}

	return 0;
}

/**
	Finds free block in bitmap and sets it to taken
	returns 0 - no block is free
		block - position of block (indexing from 0)
*/
int32_t allocate_free_block() {
	int32_t block = 0;
	uint8_t byte = 0x00;
	int i = 0;

	fseek(fs_file, sblock->bitmap_start_address, SEEK_SET);
	
	while(block < sblock->cluster_count) {
		byte = fgetc(fs_file);
		for(i = (sizeof(uint8_t)) * 8 - 1; i >= 0; i--) {
			if(!(byte & (1 << i))) {
				byte |= (1 << i);
				fseek(fs_file, -1, SEEK_CUR);
				fputc(byte, fs_file);

				return block;
			}

			block++;
		}
	}

	return 0;
}


/**
	Sets bit @position in inode bitmap to 0
*/
int free_inode_in_bm_at(int32_t position) {
	int from_byte = position / 8;
	int bit = position % 8;
	uint8_t byte = 0x00;

	fseek(fs_file, sblock->bitmapi_start_address + from_byte * sizeof(uint8_t), SEEK_SET);
	byte = fgetc(fs_file);

	byte &= ~(1 << (7 - bit));
	fseek(fs_file, -1, SEEK_CUR);
	fputc(byte, fs_file);

	return 0;
}

/**
	Sets bit @position in block bitmap to 0
*/
int free_block_in_bm_at(int32_t position) {
	int from_byte = position / 8;
	int bit = position % 8;
	uint8_t byte = 0x00;

	fseek(fs_file, sblock->bitmap_start_address + from_byte * sizeof(uint8_t), SEEK_SET);
	byte = fgetc(fs_file);

	byte &= ~(1 << (7 - bit));
	fseek(fs_file, -1, SEEK_CUR);
	fputc(byte, fs_file);

	return 0;
}


/**************************************/
/* 				      */
/*	Support functions -	      */
/*		command specific      */
/*				      */
/**************************************/


/**
	Goes through block for @node loading directory_item structure until it finds 0
	or reaches maximum number of directory_item for one block. Appends @di (new
	directory_item) after last element (unless maximum number is reached)
*/
int append_dir_item(directory_item *di, inode *node) {
	int counter = 0;
	directory_item *temp = calloc(1, sizeof(directory_item));
	return_error_on_condition(!temp, MEMORY_ALLOCATION_ERROR_MESSAGE, OUT_OF_MEMORY_ERROR);

	printf("di name: %s\n", di->item_name);
	
	//TODO: make sure its not direct1 - 1
	//fseek(fs_file, sblock->data_start_address + (node->direct1 * sblock->cluster_size), SEEK_SET);
	fseek(fs_file, get_block_address_from_position(node->direct1), SEEK_SET);
	fread(temp, sizeof(directory_item), 1, fs_file);

	while(temp->inode && counter < MAX_DIR_ITEMS_IN_BLOCK) {
		fread(temp, sizeof(directory_item), 1, fs_file);
		counter++;
	}

	if(counter >= MAX_DIR_ITEMS_IN_BLOCK) {
		printf("CANNOT CREATE ANOTHER DIRECTORY");
		//print error
		free(temp);
		return 1;
	}

	fseek(fs_file, -sizeof(directory_item), SEEK_CUR);
	fwrite(di, sizeof(directory_item), 1, fs_file);

	free(temp);
	
	return 0;
}


/**************************************/
/* 				      */
/*	Command functions	      */
/*				      */
/**************************************/


/**
	Searches for name in directory with inodeid = from_nid
*/
int search_dir(char *name, int32_t from_nid) {
	int item_counter = 0;

	inode *nd = malloc(sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	directory_item *di = malloc(sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	fseek(fs_file, get_inode_address_from_id(from_nid), SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);

	if(nd->isDirectory != 1) { //cannot search something that isn't dir
		free(nd);
		free(di);
		return 2;
	}

	//fseek(fs_file, sblock->data_start_address + nd->direct1 * sblock->cluster_size, SEEK_SET);
	fseek(fs_file, get_block_address_from_position(nd->direct1), SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	while(di->inode != 0 && item_counter < MAX_DIR_ITEMS_IN_BLOCK) {
		if(strcmp(di->item_name, name)) {
			fread(di, sizeof(directory_item), 1, fs_file);
			item_counter++;
		} else {
			//*from_nid = di->inode;
			break;
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

int does_item_exist_in_dir(char *name, int32_t dir_id) {
	int item_counter = 0;

	inode *nd = malloc(sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	directory_item *di = malloc(sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	fseek(fs_file, get_inode_address_from_id(dir_id), SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);

	if(nd->isDirectory != 1) {
		free(nd);
		free(di);
		return 0;
	}

	//fseek(fs_file, sblock->data_start_address +
	 //     nd->direct1 * sblock->cluster_size, SEEK_SET);
	fseek(fs_file, get_block_address_from_position(nd->direct1), SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	while(di->inode != 0 && item_counter < MAX_DIR_ITEMS_IN_BLOCK) {
		if(strcmp(di->item_name, name)) {
			fread(di, sizeof(directory_item), 1, fs_file);
			item_counter++;
		} else {
			free(nd);
			free(di);

			return 1;
		}
	}

	//no dir item with name found
	if(!di->inode) {
		free(nd);
		free(di);
		 
		return 0;// ?????
		//return 1;
	}
	
	free(nd);
	free(di);

	return 0;
}

directory_item *find_dir_item_by_id(inode *nd, int32_t node_id) {
	int item_counter = 0;

	//TODO: free nd
	directory_item *di = calloc(1, sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL); 

	fseek(fs_file, get_block_address_from_position(nd->direct1), SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	while(di->inode != node_id && di->inode &&
	      item_counter < MAX_DIR_ITEMS_IN_BLOCK) {
		fread(di, sizeof(directory_item), 1, fs_file);
		item_counter++;
	}

	if(di->inode /* == node_id */) return di;

	return NULL;
}
