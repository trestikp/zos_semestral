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
/*	Functions		      */
/*				      */
/**************************************/

void print_inode_bitmap() {
	int32_t address = 0;
	uint8_t byte = 0x00;
	int i = 0;

	fseek(fs_file, sblock->bitmapi_start_address, SEEK_SET);

	printf("Inode bitmap: ");
	while(address < ((sblock->bitmap_start_address - sblock->bitmapi_start_address) * 8)) {
		byte = fgetc(fs_file);

		for(i = 7; i >= 0; i--) {
			if(byte & (1 << i)) {
				printf("1");
			} else {
				printf("0");
			}

			address++;
		}

		printf(" ");
	}
	printf("\n");
}


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


void save_inode(inode *nd) {
	fseek(fs_file, get_inode_address_from_id(nd->nodeid), SEEK_SET);
	fwrite(nd, sizeof(inode), 1, fs_file);
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

	printf("ERROR: Out of inodes\n");

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

	return -1;
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


int count_free_blocks() {
	int count = 0, i = 0;
	uint8_t byte = 0x00;
	int32_t block = 0;

	fseek(fs_file, sblock->bitmap_start_address, SEEK_SET);

	//TODO this might be broken
	while(block < sblock->cluster_count) {
		byte = fgetc(fs_file);
		for(i = 7; i >= 0; i--) {
			if(!(byte & (1 << i))) count++;
			block++;
		}
	}

	//printf("Counted %d free blocks out of %d max\n", count, sblock->cluster_count);
	
	return count;
}


/**
	Goes through block for @node loading directory_item structure until it finds 0
	or reaches maximum number of directory_item for one block. Appends @di (new
	directory_item) after last element (unless maximum number is reached)
*/
int append_dir_item(directory_item *di, inode *node) {
	int counter = 0;

	directory_item *temp = calloc(1, sizeof(directory_item));
	return_error_on_condition(!temp, MEMORY_ALLOCATION_ERROR_MESSAGE, OUT_OF_MEMORY_ERROR);

	//printf("di name: %s\n", di->item_name);
	
	//fseek(fs_file, sblock->data_start_address + (node->direct1 * sblock->cluster_size), SEEK_SET);
	fseek(fs_file, get_block_address_from_position(node->direct1), SEEK_SET);
	fread(temp, sizeof(directory_item), 1, fs_file);

	while(temp->inode && counter < MAX_DIR_ITEMS_IN_BLOCK) {
		fread(temp, sizeof(directory_item), 1, fs_file);
		counter++;
	}

	if(counter >= MAX_DIR_ITEMS_IN_BLOCK) {
		printf("CANNOT CREATE ANOTHER DIRECTORY\n");
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
int search_dir(char *name, int32_t *from_nid) {
	int item_counter = 0;

	inode *nd = calloc(1, sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	directory_item *di = calloc(1, sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	fseek(fs_file, get_inode_address_from_id(*from_nid), SEEK_SET);
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
			*from_nid = di->inode;
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


/**
	Overwrites inode with @node_id with 0, resulting in removal of the inode
*/
int free_inode_with_id(int32_t node_id) {	
	int i = 0;

	free_inode_in_bm_at(node_id - 1);

	fseek(fs_file, get_inode_address_from_id(node_id), SEEK_SET);
		
	for(i = 0; i < sizeof(inode); i++) {
		fputc(0x00, fs_file);
	}

	return 0;
}


void write_buffer_to_block(char buffer[sblock->cluster_size], int32_t block) {
	fseek(fs_file, get_block_address_from_position(block), SEEK_SET);
	fwrite(buffer, sizeof(char), sblock->cluster_size, fs_file);
}


void write_data_to_block(char buffer[BLOCK_SIZE], inode *nd, int block_num) {
	int32_t block = 0;

	//number of blocks that fit in one block
	int number_of_indirects = (sblock->cluster_size / sizeof(int32_t));

	switch(block_num) {
		case 1: write_buffer_to_block(buffer, nd->direct1); break;
		case 2: write_buffer_to_block(buffer, nd->direct2); break;
		case 3: write_buffer_to_block(buffer, nd->direct3); break;
		case 4: write_buffer_to_block(buffer, nd->direct4); break;
		case 5: write_buffer_to_block(buffer, nd->direct5); break;
	}

	if(block_num > 5 && block_num <= (number_of_indirects + 5)) {
		fseek(fs_file, get_block_address_from_position(nd->indirect1) +
			(block_num - 6) * sizeof(int32_t), SEEK_SET);
		fread(&block, sizeof(int32_t), 1, fs_file);

		write_buffer_to_block(buffer, block);
	} else if(block_num > (number_of_indirects + 5)) {
		int div = (block_num - 6) / number_of_indirects - 1; //get indirect layer 
		int mod = (block_num - 6) % number_of_indirects; //get position in indirect layer
		int32_t outer = 0;

		fseek(fs_file, get_block_address_from_position(nd->indirect2) + div * sizeof(int32_t), SEEK_SET);
		fread(&outer, sizeof(int32_t), 1, fs_file);

		fseek(fs_file, get_block_address_from_position(outer) + mod * sizeof(int32_t), SEEK_SET);
		fread(&block, sizeof(int32_t), 1, fs_file);

		if(!outer || !block) {
			printf("ERROR: Canno't write to position 0\n");
			return;
		}

		write_buffer_to_block(buffer, block);
	}
}


void read_block_to_buffer(char buffer[sblock->cluster_size], int32_t block) {
	fseek(fs_file, get_block_address_from_position(block), SEEK_SET);
	fread(buffer, sizeof(char), sblock->cluster_size, fs_file);
}

int read_data_to_buffer(char buffer[BLOCK_SIZE], inode *nd, int32_t block_num) {
	int32_t block = 0;	

	//number of blocks that fit in one block
	int number_of_indirects = (sblock->cluster_size / sizeof(int32_t));

	switch(block_num) {
		case 1: block = nd->direct1; break;
		case 2: block = nd->direct2; break;
		case 3: block = nd->direct3; break;
		case 4: block = nd->direct4; break;
		case 5: block = nd->direct5; break;
	}

	if(block_num > 5 && block_num <= (number_of_indirects + 5)) {
		fseek(fs_file, get_block_address_from_position(nd->indirect1) +
			(block_num - 6) * sizeof(int32_t), SEEK_SET);
		fread(&block, sizeof(int32_t), 1, fs_file);
	} else if(block_num > (number_of_indirects + 5)) {
		int div = (block_num - 6) / number_of_indirects - 1; //get indirect layer
		int mod = (block_num - 6) % number_of_indirects; //get position in indirect layer
		int32_t outer = 0;

		fseek(fs_file, get_block_address_from_position(nd->indirect2) +
			div * sizeof(int32_t), SEEK_SET);
		fread(&outer, sizeof(int32_t), 1, fs_file);

		fseek(fs_file, get_block_address_from_position(outer) + mod * sizeof(int32_t), SEEK_SET);
		fread(&block, sizeof(int32_t), 1, fs_file);// TODO will this work?
	}

	if(block) {
		//printf("\n\nREADING DATA FROM BLOCK %d\n\n", block);

		read_block_to_buffer(buffer, block);

		/*
		if((nd->file_size - ((block_num-1) * sblock->cluster_size)) < sblock->cluster_size) {
			//buffer[nd->file_size - ((block_num-1) * sblock->cluster_size)] = 0x00;
		}
		*/

		return 0;
	} else return 1;
}


void zero_data_block(int32_t block) {
	int i = 0; 

	fseek(fs_file, get_block_address_from_position(block), SEEK_SET);

	for(i = 0; i < sblock->cluster_size; i++) {
		fputc(0x00, fs_file);
	}
}


int allocate_blocks_for_file(inode *nd, int block_count) {
	int i = 0, cnt = 0, additional = 0;
	int32_t blocks[sblock->cluster_size / sizeof(int32_t)];


	if(block_count <= 0) {
		printf("ERROR: Block count mustn't be <= 0\n");
		return 1;
	}

	cnt = count_free_blocks(); //figured counting is faster then allocating and freeing on failure

	//number of additional blocks needed for indirect
	additional = block_count / (sblock->cluster_size / sizeof(int32_t)); //TODO probably not accurate

	if(additional > MAX_NUMBER_OF_ADDITIONAL) {
		printf("ERROR: Cannot allocate that many blocks. What MONSTROUS file do you have there?\n");
		return 3;
	}

	if((cnt + additional) <= block_count) {
		printf("ERROR: Not enough blocks to store data\n");
		return 2;
	}

	// don't need to check for allocate result, because count_free_block
	// asures that there is enough free blocks (if it works :D)
	if(block_count >= 1) nd->direct1 = allocate_free_block();
	if(block_count >= 2) nd->direct2 = allocate_free_block();
	if(block_count >= 3) nd->direct3 = allocate_free_block();
	if(block_count >= 4) nd->direct4 = allocate_free_block();
	if(block_count >= 5) nd->direct5 = allocate_free_block();

	if(block_count >= 6) {
		block_count -= 5;

	//indirect 1
		nd->indirect1 = allocate_free_block();
		zero_data_block(nd->indirect1);

		if(block_count > (sblock->cluster_size / sizeof(int32_t)))
			cnt = sblock->cluster_size / sizeof(int32_t);
		else cnt = block_count;

		for(i = 0; i < cnt; i++) {
			blocks[i] = allocate_free_block();
		}

		fseek(fs_file, get_block_address_from_position(nd->indirect1), SEEK_SET);
		fwrite(blocks, sizeof(int32_t), cnt, fs_file);

		block_count -= cnt;
	//indirect 1 end

	// do indirect2
		if(block_count > 0) { 
			nd->indirect2 = allocate_free_block();
			zero_data_block(nd->indirect2);

			int iter = 0;
			int32_t outers[BLOCK_SIZE / sizeof(int32_t)] = {0};

			while(block_count > 0) {
				if(block_count > (sblock->cluster_size / sizeof(int32_t)))
					cnt = sblock->cluster_size / sizeof(int32_t);
				else cnt = block_count;
	
				for(i = 0; i < cnt; i++) {
					blocks[i] = allocate_free_block();
				}
	
				outers[iter] = allocate_free_block();
				zero_data_block(outers[iter]);

				fseek(fs_file, get_block_address_from_position(outers[iter]), SEEK_SET);
				fwrite(blocks, sizeof(int32_t), cnt, fs_file);

				block_count -= cnt;
				iter++;
			}

			fseek(fs_file, get_block_address_from_position(nd->indirect2), SEEK_SET);
			fwrite(outers, sizeof(int32_t), BLOCK_SIZE / sizeof(int32_t), fs_file);
		}

		save_inode(nd);
	}
	
	return 0;
}


/**
	Sets blocks in bitmap allocated by @nd to 0 -> "removing" data
*/
int free_allocated_blocks(inode *nd) {
	int count = 0;
	int32_t block = 0;

	if(nd->direct1) free_block_in_bm_at(nd->direct1);
	if(nd->direct2) free_block_in_bm_at(nd->direct2);
	if(nd->direct3) free_block_in_bm_at(nd->direct3);
	if(nd->direct4) free_block_in_bm_at(nd->direct4);
	if(nd->direct5) free_block_in_bm_at(nd->direct5);
	if(nd->indirect1) {
		fseek(fs_file, get_block_address_from_position(nd->indirect1), SEEK_SET);
		fread(&block, sizeof(int32_t), 1, fs_file);

		while(block && count < (sblock->cluster_size / sizeof(int32_t))) {
			free_block_in_bm_at(block);
			fread(&block, sizeof(int32_t), 1, fs_file);
			count++;
		}

		free_block_in_bm_at(nd->indirect1);
	}
	if(nd->indirect2) { //TODO for now lets assume this works (debug it)
		int32_t outer = 0;
		int count_out = 1; //because first is loaded before while

		fseek(fs_file, get_block_address_from_position(nd->indirect2), SEEK_SET);
		fread(&outer, sizeof(int32_t), 1, fs_file);

		while(outer && count_out < (sblock->cluster_size / sizeof(int32_t))) {
			fseek(fs_file, get_block_address_from_position(outer), SEEK_SET);
			fread(&block, sizeof(int32_t), 1, fs_file);

			count = 0;

			while(block != 0 && count < (sblock->cluster_size / sizeof(int32_t))) {
				free_block_in_bm_at(block);
				fread(&block, sizeof(int32_t), 1, fs_file);
				count++;
			}

			free_block_in_bm_at(outer);

			fseek(fs_file, get_block_address_from_position(outer) + count_out * sizeof(int32_t), SEEK_SET);
			fread(&outer, sizeof(int32_t), 1, fs_file);
		}

		free_block_in_bm_at(nd->indirect2);
	}

	nd->direct1 = 0;
	nd->direct2 = 0;
	nd->direct3 = 0;
	nd->direct4 = 0;
	nd->direct5 = 0;
	nd->indirect1 = 0;
	nd->indirect2 = 0;
	
	return 0;
}

directory_item *find_dir_item_by_id(inode *nd, int32_t node_id) {
	int item_counter = 0;

	//TODO: free nd
	directory_item *di = calloc(1, sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL); 

	fseek(fs_file, get_block_address_from_position(nd->direct1), SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	while(di->inode != node_id && di->inode && item_counter < MAX_DIR_ITEMS_IN_BLOCK) {
		fread(di, sizeof(directory_item), 1, fs_file);
		item_counter++;
	}

	if(di->inode /* == node_id */) return di;

	return NULL;
}


/**
	Checks if dir has a directory_item other than "." and ".."
	return 0 on empty, 1 on not empty, 2 on memory error
*/
int is_dir_empty(inode *nd) {
	directory_item *di = calloc(1, sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 2);

	fseek(fs_file, get_block_address_from_position(nd->direct1) + sizeof(directory_item) * 2, SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	if(!di->inode) return 0;

	return 1;
}


/**
	Removes inode @nd and removes this inode from parents directory_items
*/
int remove_dir_node(inode *nd) {
	int32_t parent_id = nd->nodeid;
	int count = 0, i = 0;
	inode *parent = NULL;
	directory_item *di = calloc(1, sizeof(directory_item));
	link *head = NULL;
	
	if(search_dir("..", &parent_id)) {
		printf("Error: Failed to find parent dir");
		return 1;
	}

	parent = load_inode_by_id(parent_id);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);
	
	// skip "." and ".."
	fseek(fs_file, get_block_address_from_position(parent->direct1) + sizeof(directory_item) * 2, SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	while(di->inode && count < MAX_DIR_ITEMS_IN_BLOCK) {
		if(di->inode != nd->nodeid) add_fifo(&head, di);

		fread(di, sizeof(directory_item), 1, fs_file);
		count++;
	}

	fseek(fs_file, get_block_address_from_position(parent->direct1) + sizeof(directory_item) * 2, SEEK_SET);
	while(head) {
		fwrite(head->data, sizeof(directory_item), 1, fs_file);

		free(head->data);
		link *temp = head;
		head = head->next;
		free(temp);
	}

	//there should be one item, that isn't overwritten because of the removal
	//this sets it to 0
	for(i = 0; i < sizeof(directory_item); i++) {
		fputc(0x00, fs_file);
	}


	// a lot of IO operation solution
	/*
	fread(di, sizeof(directory_item), 1, fs_file);

	while(di->inode != nd->nodeid) {
		fread(di, sizeof(directory_item), 1, fs_file);
	}

	fread(di, sizeof(directory_item), 1, fs_file);
	do {
		fseek(fs_file, -2 * sizeof(directory_item), SEEK_CUR);
		fwrite(di, sizeof(directory_item), 1, fs_file);
		fseek(fs_file, sizeof(directory_item), SEEK_CUR);
		fread(di, sizeof(directory_item), 1, fs_file);
	} while(di->inode != 0);
	*/
	
	free_block_in_bm_at(nd->direct1);
	free_inode_with_id(nd->nodeid);

	return 0;
}

int remove_dir_node_2(inode *nd) {
	int32_t parent_id = nd->nodeid;
	inode *parent = NULL;
	int i = 0, count = 0, passed = 0;
	directory_item content[MAX_DIR_ITEMS_IN_BLOCK] = {0}, di;

	//printf("Removing dir with nodeid %d\n", nd->nodeid);

	if(search_dir("..", &parent_id)) {
		printf("ERROR: Failed to find parent dir\n");
		return 1;
	}

	parent = load_inode_by_id(parent_id);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	for(i = 0; i < MAX_NUMBER_OF_ADDITIONAL; i++) {
		fread(&di, sizeof(directory_item), 1, fs_file);

		if(!di.inode) break;

		if(passed) {
			content[i - 1] = di;
		} else {
			if(di.inode == nd->nodeid) {
				passed = 1;
			} else {
				content[i] = di;
			}
		}

		count++;
	}

	//printf("Removing dir item from nodeid %d\n COUNT: %d\n", parent->nodeid, count);

	zero_data_block(parent->direct1);
	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	fwrite(content, sizeof(directory_item), count, fs_file);

	free_allocated_blocks(nd);
	free_inode_with_id(nd->nodeid);
	
	return 0;
}

int remove_dir_item(int32_t parent_nid, int32_t di_nid) {
	int i = 0, count = 0, passed = 0;
	directory_item content[MAX_DIR_ITEMS_IN_BLOCK] = {0}, di;

	inode *parent = load_inode_by_id(parent_nid);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	inode *nd = load_inode_by_id(di_nid);
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	if(parent->isDirectory != 1) {
		printf("ERROR: Parent isn't a dir");
		return 1;
	}

	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	for(i = 0; i < MAX_NUMBER_OF_ADDITIONAL; i++) {
		fread(&di, sizeof(directory_item), 1, fs_file);

		if(!di.inode) break;

		if(passed) {
			content[i - 1] = di;
		} else {
			if(di.inode == nd->nodeid) {
				passed = 1;
			} else {
				content[i] = di;
			}
		}

		count++;
	}

	//printf("Removing dir item from nodeid %d\n COUNT: %d\n", parent->nodeid, count);

	zero_data_block(parent->direct1);
	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	fwrite(content, sizeof(directory_item), count, fs_file);

	free_allocated_blocks(nd);
	free_inode_with_id(nd->nodeid);

	return 0;
}

directory_item *extract_dir_item_from_dir(int32_t where, int32_t item_id) { 
	int i = 0, count = 0, passed = 0;
	directory_item content[MAX_DIR_ITEMS_IN_BLOCK] = {0}, di;
	directory_item *res = calloc(1, sizeof(directory_item));


	inode *parent = load_inode_by_id(where);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);

	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	for(i = 0; i < MAX_NUMBER_OF_ADDITIONAL; i++) {
		fread(&di, sizeof(directory_item), 1, fs_file);

		if(!di.inode) break;

		if(passed) {
			content[i - 1] = di;
		} else {
			if(di.inode == item_id) {
				passed = 1;
				memcpy(res, &di, sizeof(directory_item));
			} else {
				content[i] = di;
			}
		}

		count++;
	}


	zero_data_block(parent->direct1);
	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	fwrite(content, sizeof(directory_item), count, fs_file);

	free(parent);

	return res;
}

directory_item *extract_dir_item_from_dir_2(inode *nd) { 
	int32_t parent_id = nd->nodeid;
	inode *parent = NULL;
	int i = 0, count = 0, passed = 0;
	directory_item content[MAX_DIR_ITEMS_IN_BLOCK] = {0}, di;
	directory_item *res = calloc(1, sizeof(directory_item));

	if(search_dir("..", &parent_id)) {
		printf("ERROR: Failed to find parent dir");
		return NULL;
	}

	parent = load_inode_by_id(parent_id);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);

	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	for(i = 0; i < MAX_NUMBER_OF_ADDITIONAL; i++) {
		fread(&di, sizeof(directory_item), 1, fs_file);

		if(!di.inode) break;

		if(passed) {
			content[i - 1] = di;
		} else {
			if(di.inode == nd->nodeid) {
				passed = 1;
				memcpy(res, &di, sizeof(directory_item));
			} else {
				content[i] = di;
			}
		}

		count++;
	}

	zero_data_block(parent->direct1);
	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	fwrite(content, sizeof(directory_item), count, fs_file);

	free(parent);

	return res;
}


uint64_t get_dir_item_address(inode *parent, int32_t id) {
	int i = 0;
	directory_item di;

	fseek(fs_file, get_block_address_from_position(parent->direct1), SEEK_SET);
	for(i = 0; i < MAX_DIR_ITEMS_IN_BLOCK; i++) {
		fread(&di, sizeof(directory_item), 1, fs_file);
		if(di.inode == id) {
			fseek(fs_file, -sizeof(directory_item), SEEK_CUR);
			return ftell(fs_file);
		}
	}

	return 0;
}

int load_linked_node(inode **nd) {
	if((*nd)->isDirectory == 2) {
		int32_t new_id = (*nd)->direct1;

		free((*nd));

		(*nd) = load_inode_by_id(new_id);
		return_error_on_condition(!(*nd), MEMORY_ALLOCATION_ERROR_MESSAGE, 1);
	} else return 2;

	return 0;
}
