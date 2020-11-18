/**************************************/
/* 				      */
/*	Includes		      */
/*				      */
/**************************************/

#include "commands.h"

/**************************************/
/* 				      */
/*	Global Variables	      */
/*				      */
/**************************************/

extern superblock *sblock;
extern FILE *fs_file;
extern const inode *root;
extern const inode *position;


/**************************************/
/* 				      */
/*	Commands		      */
/*				      */
/**************************************/

int32_t traverse_path(char *path) {
        char *token = NULL, *path_cpy = NULL;
        int ret = 0, on_way = 0;
        int32_t node_id = 0;

	printf("IM HERE\n");

        if(path == NULL) {
                path = malloc(2 * sizeof(char));
                path[0] = '.';
                path[1] = 0x00;
        }

        if(path[0] == '/') {
                node_id = root->nodeid;
                //printf("ROOD ID: %d\n", root->nodeid);
        } else {
                node_id = position->nodeid;
                //printf("POSITION ID: %d\n", node_id);
        }

	printf("NOICE\n");

        path_cpy = malloc(strlen(path) + 1);
        strcpy(path_cpy, path);

        token = strtok(path_cpy, "/");
        while(token) {
                switch(ret = search_dir(token, &node_id)){
                        case 0: printf("zero\n"); break;
                        case 1: printf("doesn't exsit\n"); break;
                        case 2: printf("isn't dir\n"); break;
                        default: ret = 1; printf("unknown error\n");
                }

                if(ret) {
                        on_way = 1;
                }

                token = strtok(NULL, "/");
        }

        if(on_way) {
                return 0;
        }

        free(path_cpy);

        return node_id;
}

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


/****************/
/*		*/
/*    format	*/
/*		*/
/****************/

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
	return_error_on_condition(!node, MEMORY_ALLOCATION_ERROR_MESSAGE, 2);

	node->nodeid = 1;
	node->isDirectory = 1;
	node->references = 1;
	node->direct1 = 0;
	node->file_size = BLOCK_SIZE;

	printf("INODE WRITE: %ld\n", ftell(fs_file));
	fwrite(node, sizeof(inode), 1, fs_file);

	node->references = 0;
	node->isDirectory = 0;
	node->file_size = 0;

	for(i = 1; i < ((sblock->bitmap_start_address -
	sblock->bitmapi_start_address) * 8); i++) {
		node->nodeid = i + 1;
		fwrite(node, sizeof(inode), 1, fs_file);
	}

	free(node);

	//first block
	ditem = malloc(sizeof(directory_item));
	return_error_on_condition(!ditem, MEMORY_ALLOCATION_ERROR_MESSAGE, 2);

	ditem->inode = 1;
	/*
	bzero(ditem->item_name, MAX_ITEM_NAME_LENGTH);
	ditem->item_name[0] = '/';
	*/

	printf("BLOCK WRITE: %ld\n", ftell(fs_file));
	//fwrite(ditem, sizeof(directory_item), 1, fs_file);

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

/****************/
/*		*/
/*    mkdir	*/
/*		*/
/****************/

int make_directory(char *name, int32_t parent_nid) {
	//position is equivalent to nodeid
	int32_t pos = 0;
	inode *new = NULL, *parent = NULL;

	/*
	inode *new = malloc(sizeof(inode));
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);
	 */

	directory_item *di = malloc(sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);

	pos = allocate_free_inode();
	new = load_inode_by_id(pos);
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);
	parent = load_inode_by_id(parent_nid);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);

	new->direct1 = allocate_free_block();
	//TODO: free memory on condition!!
	return_error_on_condition(!new->direct1, "failed to allocate free block", 2);

	new->nodeid = pos;
	new->isDirectory = 1;
	new->file_size = sblock->cluster_size;
	new->references = 1;

	//fseek(fs_file, -(sizeof(inode)), SEEK_CUR);
	fseek(fs_file, (sblock->inode_start_address +
	      (pos - 1) * sizeof(inode)), SEEK_SET); //TODO: make sure pos - 1
	fwrite(new, sizeof(inode), 1, fs_file);
	
	//write to parent dir
	di->inode = new->nodeid;
	memcpy(di->item_name, name, MAX_ITEM_NAME_LENGTH - 1);
	append_dir_item(di, parent);

	//write to new dir
	bzero(di->item_name, MAX_ITEM_NAME_LENGTH);
	memcpy(di->item_name, "..", 1);
	append_dir_item(di, new);
	//fwrite(di, sizeof(directory_item), 1, fs_file);

	di->inode = parent_nid;
	memcpy(di->item_name, "..", 2);
	append_dir_item(di, new);
	//fwrite(di, sizeof(directory_item), 1, fs_file);

	return 0;
}

/****************/
/*		*/
/*    ls	*/
/*		*/
/****************/

void print_ls_record(inode *nd, char *name) {
	switch(nd->isDirectory) {
		case 0: printf("f\t"); break;
		case 1: printf("d\t"); break;
		case 2: printf("l\t"); break;
	}

	printf("%d\t%d\t%s\n", nd->file_size, nd->nodeid, name);
}

int list_dir_contents(int32_t node_id) {
	uint64_t address = sblock->inode_start_address + (node_id - 1) * sizeof(inode);
	int item_counter = 0, file_pos = 0;
	inode *tmp = NULL;
	link *head = NULL;

	//printf("STARTING FROM: %d\n", node_id);

	inode *nd = malloc(sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	directory_item *di = malloc(sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	fseek(fs_file, address, SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);

	fseek(fs_file, sblock->data_start_address + nd->direct1 * sblock->cluster_size, SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	printf("type\tsize\tinode\tname\n");

	while(di->inode != 0 && item_counter < MAX_DIR_ITEMS_IN_BLOCK) {
		file_pos = ftell(fs_file);

		nd = load_inode_by_id(di->inode);

		print_ls_record(nd, di->item_name);

		fseek(fs_file, file_pos, SEEK_SET);
		fread(di, sizeof(directory_item), 1, fs_file);

		item_counter++;
	}

	return 0;
}
