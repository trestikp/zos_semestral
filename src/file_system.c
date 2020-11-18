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

int append_dir_item(directory_item *di, inode *node) {
	int counter = 0;
	directory_item *temp = malloc(sizeof(directory_item));
	return_error_on_condition(!temp, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);

	fseek(fs_file, sblock->data_start_address +
	      (node->direct1 * sblock->cluster_size), SEEK_SET); //TODO: make sure its not direct1 - 1
	fread(temp, sizeof(directory_item), 1, fs_file);

	while(temp->inode && counter < MAX_DIR_ITEMS_IN_BLOCK) {
		fread(temp, sizeof(directory_item), 1, fs_file);
		counter++;
	}

	if(counter >= MAX_DIR_ITEMS_IN_BLOCK) {
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
