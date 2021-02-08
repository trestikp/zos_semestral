/**************************************/
/* 				      */
/*	Includes		      */
/*				      */
/**************************************/

#include "commands.h"
#include "file_system.h"

/**************************************/
/* 				      */
/*	Global Variables	      */
/*				      */
/**************************************/

extern superblock *sblock;
extern FILE *fs_file;
extern const inode *root;
extern inode *position;

/**************************************/
/* 				      */
/*	Commands		      */
/*				      */
/**************************************/


/**
	Goes through given @path
	return node_id - of last element in path
		0 - on error
*/
int32_t traverse_path(char *path) {
        char *token = NULL, *path_cpy = NULL;
        int ret = 0, on_way = 0;
        int32_t node_id = 0;

        if(path == NULL) {
		path = calloc(2, sizeof(char));

		if(!path) return -1;

                path[0] = '.';
                path[1] = 0x00;
		//TODO: FREE THIS
        }

        if(path[0] == '/') {
                node_id = root->nodeid;
                printf("ROOD ID: %d\n", root->nodeid);
        } else {
                node_id = position->nodeid;
                printf("POSITION ID: %d\n", node_id);
        }

	path_cpy = calloc(strlen(path) + 1, sizeof(char));
	if(!path_cpy) return 0;
        strcpy(path_cpy, path);

        token = strtok(path_cpy, "/");
        while(token) {
		if(on_way) {
			on_way = 2;
			break;
		}

		ret = search_dir(token, &node_id);

		if(ret == 1) {
			printf("FILE NOT FOUND\n");
			break;
		} else if(ret == 2) {
			on_way = 1;
		} else if(ret) {
			printf("UKNOWN SEARCH DIR ERROR\n");
			free(path_cpy);
			return 0;
		}
		/*
                switch(ret = search_dir(token, &node_id)){
                        case 0: //printf("zero\n"); //break; // 0 = all good
                        case 1: printf("FILE NOT FOUND\n"); break;
                        case 2: printf("CANNOT TRAVERSE FILE\n"); break;
                        default: ret = 1; printf("UNKNOWN ERROR\n");
                }

                if(ret) {
                        on_way = 1;
                }
		*/

                token = strtok(NULL, "/");
        }

        if(on_way == 2) {
		free(path_cpy);
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
	Returns superblock structure
*/
superblock *create_superblock(uint64_t max_size) {
	int ibmp_size = 0, bbmp_size = 0;

	superblock *sb = calloc(1, sizeof(superblock));
	return_error_on_condition(!sb, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);
	
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

	//fill rest of free space with blocks
	while((size + BLOCK_SIZE + 1) <= max_size) {
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
	sb->inode_start_address = sb->bitmap_start_address + bbmp_size;
	//sb->inode_start_address = sb->bitmapi_start_address + ibmp_size + bbmp_size;
	sb->data_start_address = sb->inode_start_address + (sizeof(inode) * (ibmp_size * 8));
	//sb->data_start_address = sb->bitmapi_start_address + ibmp_size + bbmp_size + (sizeof(inode) * (ibmp_size * 8));
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
	return_error_on_condition(!sblock, SUPERBLOCK_CREATION_ERROR, 1);

	print_superblock(sblock);

	fseek(fs_file, 0, SEEK_SET);
	fwrite(sblock, sizeof(superblock), 1, fs_file);

	printf("IBMP WRITE: %ld\n", ftell(fs_file));
	//first inode bmp byte (root)
	fputc(0x80, fs_file);
	//fwrite? 
	for(i = 0; i < (sblock->bitmap_start_address - sblock->bitmapi_start_address - 1); i++) {
		fputc(0x00, fs_file);
	}

	printf("BBMP WRITE: %ld\n", ftell(fs_file));
	//first block bmp byte(root)
	fputc(0x80, fs_file);
	for(i = 0; i < (sblock->inode_start_address - sblock->bitmap_start_address - 1); i++) {
		fputc(0x00, fs_file);
	}

	//first inode
	node = calloc(1, sizeof(inode));
	return_error_on_condition(!node, MEMORY_ALLOCATION_ERROR_MESSAGE, 2);

	node->nodeid = 1;
	node->isDirectory = 1;
	node->references = 1;
	node->file_size = BLOCK_SIZE;

	printf("INODE WRITE: %ld\n", ftell(fs_file));
	fwrite(node, sizeof(inode), 1, fs_file);

	/*
	node->references = 0;
	node->isDirectory = 0;
	node->file_size = 0;
	*/
	memset(node, 0x00, sizeof(inode));

	//why do i do *8?? i dont remember anymore TODO
	for(i = 1; i < ((sblock->bitmap_start_address -	sblock->bitmapi_start_address) * 8); i++) {
		node->nodeid = i + 1;
		fwrite(node, sizeof(inode), 1, fs_file);
	}

	free(node);

	//first block
	ditem = calloc(1, sizeof(directory_item));
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

	//im confused, aren't there only 2 dir_items?? TODO (*2 instead *3)
	for(i = 0; i < (sblock->cluster_size * sblock->cluster_count - sizeof(directory_item) * 3); i++) {
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
	int32_t pos = 0;
	inode *new = NULL, *parent = NULL;

	/*
	inode *new = malloc(sizeof(inode));
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE,
				  OUT_OF_MEMORY_ERROR);
	 */

	directory_item *di = malloc(sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, OUT_OF_MEMORY_ERROR);

	pos = allocate_free_inode();
	new = load_inode_by_id(pos + 1); //id is starting from 1, pos starts from 0 -> pos + 1 = id
	return_error_on_condition(!new, MEMORY_ALLOCATION_ERROR_MESSAGE, OUT_OF_MEMORY_ERROR);

	parent = load_inode_by_id(parent_nid);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE, OUT_OF_MEMORY_ERROR);

	new->direct1 = allocate_free_block();
	//TODO: free memory on condition!!
	return_error_on_condition(!new->direct1, "failed to allocate free block", 2);

	zero_data_block(new->direct1);

	new->nodeid = pos + 1;
	new->isDirectory = 1;
	new->file_size = sblock->cluster_size;
	new->references = 1;

	fseek(fs_file, (sblock->inode_start_address + pos * sizeof(inode)), SEEK_SET);
	fwrite(new, sizeof(inode), 1, fs_file);
	
	//write to parent dir
	di->inode = new->nodeid;
	memcpy(di->item_name, name, MAX_ITEM_NAME_LENGTH - 1);
	append_dir_item(di, parent);

	//write to new dir
	bzero(di->item_name, MAX_ITEM_NAME_LENGTH);
	memcpy(di->item_name, "..", 1);
	append_dir_item(di, new);

	di->inode = parent_nid;
	memcpy(di->item_name, "..", 2);
	append_dir_item(di, new);

	free(new);
	free(parent);

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
	//inode *tmp = NULL;
	//link *head = NULL;

	//printf("STARTING FROM: %d\n", node_id);

	inode *nd = malloc(sizeof(inode));
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	directory_item *di = malloc(sizeof(directory_item));
	return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 3);

	fseek(fs_file, address, SEEK_SET);
	fread(nd, sizeof(inode), 1, fs_file);

	fseek(fs_file, sblock->data_start_address + nd->direct1 * sblock->cluster_size, SEEK_SET);
	fread(di, sizeof(directory_item), 1, fs_file);

	printf("TYPE\tSIZE\tINODE\tNAME\n");

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

/****************/
/*		*/
/*    cd	*/
/*		*/
/****************/

int change_dir(int32_t target_id) {
	if(!target_id) {
		return 1;
	}

	free(position);

	position = load_inode_by_id(target_id);

	return 0;
}

/****************/
/*		*/
/*    pwd	*/
/*		*/
/****************/

int print_working_dir() {
	int fake_current = position->nodeid;
	int child_id = 0;
	link *pwd_head = NULL;
	inode *nd = NULL;


	if(position->nodeid == 1) { //duck it, when in root just print "/" and be done with it
		printf("/\n");
		return 0;
	}
	
	while(fake_current != 1) {
		child_id = fake_current;

		if(search_dir("..", &fake_current)) { //this sets fake_current to parent id
			printf("Error: Failed to fetch parent dir");
			return 1;
		}

		nd = load_inode_by_id(fake_current);
		return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 2);

		directory_item *di = find_dir_item_by_id(nd, child_id);
		return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 2);

		//add_lifo(&pwd_head, di->item_name);
		add_lifo(&pwd_head, di);

		free(nd);
	}

	while(pwd_head) {
		printf("/%s", ((directory_item*) pwd_head->data)->item_name);

		free(pwd_head->data);

		link *temp = pwd_head;
		pwd_head = pwd_head->next;
		free(temp);
	}
	printf("\n");
	
	return 0;
}

/****************/
/*		*/
/*    incp	*/
/*		*/
/****************/


inode *prepare_new_file_node() {
	int32_t new_nd = 0;
	inode *nd = NULL;

	printf("CREATING NEW NODE\n");
	if((new_nd = allocate_free_inode())) {
		nd = load_inode_by_id(new_nd + 1); //new_nd is position not id!!!
		return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, NULL);

		nd->nodeid = new_nd + 1;
		nd->references = 1;
		nd->isDirectory = 0;
	} else {
		printf("OUT OF INODES\n");
	}

	return nd;
}

int copy_file_to_node(FILE *f, inode *nd) {
	uint64_t size = 0;
	int i = 0, read = 0;
	char buffer[sblock->cluster_size + 1];

	buffer[sblock->cluster_size] = 0x00;

	//get file size
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	printf("File size: %ld\n", size);
	fseek(f, 0L, SEEK_SET);

	int block_count = size / sblock->cluster_size + 1;
	printf("Will need %d blocks\n",block_count);

	if(allocate_blocks_for_file(nd, block_count)) { //failed to allocate blocks
		return 1;
	}

	//nd->file_size = size;
	nd->file_size = 0;

	for(i = 1; i <= block_count; i++) {
		//wanted to do this only when read is < block size, but i'm useless and dunno
		bzero(buffer, sblock->cluster_size); 

		read = fread(buffer, sizeof(char), sblock->cluster_size, f);
		
		/*
		if(read != sblock->cluster_size) {
			bzero(buffer, sblock->cluster_size);
		}
		*/

		nd->file_size += read;

		write_data_to_block(buffer, nd, i);
	}

	return 0;
}

int in_copy(char* source, int32_t t_node, char *source_name, char *target_name) {
	int exists = 0;
	inode *nd = NULL, *parent = NULL;
	FILE *f = fopen(source, "rb");

	//test fopen
	if(!f) {
		printf("FILE NOT FOUND (missing source)");
		return 1;
	}

	//check if target is file, dir or doesn't exit. Creates file with @source_name
	//if target is directory (in target dir) or doesn't exist (in current dir)
	if(!search_dir(target_name, &t_node)) {
		exists = 1;
	}

	nd = load_inode_by_id(t_node);
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	if(exists) {
		if(nd->isDirectory == 0) {
			free_allocated_blocks(nd);
			copy_file_to_node(f, nd);
		} else if(nd->isDirectory == 1) {
			parent = nd;
			nd = prepare_new_file_node();
			copy_file_to_node(f, nd);
			save_inode(nd);

			directory_item *di = calloc(1, sizeof(directory_item));
			di->inode = nd->nodeid;
			strcpy(di->item_name, source_name);

			append_dir_item(di, parent);
		} else {
			printf("UKNOWN NODE TYPE");
			return 1;
		}
	} else {
		parent = nd;
		nd = prepare_new_file_node();

		if(copy_file_to_node(f, nd)) {
			//theorethically there isn't need to free the inode because it wasn't
			//saved yet, but just to be sure
			free_inode_with_id(nd->nodeid);
			return 1;
		}

		save_inode(nd);

		directory_item *di = calloc(1, sizeof(directory_item));
		di->inode = nd->nodeid;
		strcpy(di->item_name, source_name);

		append_dir_item(di, parent);
	}

	return 0;
}


/****************/
/*		*/
/*    rmdir	*/
/*		*/
/****************/

int remove_directory(int32_t node_id) {
	int rv = 0;

	inode *nd = load_inode_by_id(node_id);
	return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	if(nd->isDirectory != 1) {
		printf("ISN'T DIR\n");
	}

	if((rv = is_dir_empty(nd))) { //is_dir_empty return 0 on empty, number on error
		switch(rv) {
			case 1: printf("NOT EMPTY\n"); return 2;
			case 2: return 1;
		}
	}
	
	//remove_dir_node(nd);
	remove_dir_node_2(nd);

	free(nd);

	return 0;
}


/****************/
/*		*/
/*    cat	*/
/*		*/
/****************/

int cat_file(int32_t where, char *name) {
	int rv = 0, it = 1;
	char buffer[sblock->cluster_size + 1];
	
	buffer[sblock->cluster_size] = 0x00;

	if(!search_dir(name, &where)) {
		inode *nd = load_inode_by_id(where);
		return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

		while(!rv) {
			rv = read_data_to_buffer(buffer, nd, it);
			if(rv) break;
			printf("%s", buffer);
			it++;
		}
	} else {
		//printf("Failed to find %s in dir\n", name);
		return 1;
	}
	
	return 0;
}


/****************/
/*		*/
/*    rm	*/
/*		*/
/****************/

int remove_file(int32_t where, char *name) {
	int parent = where;

	if(!search_dir(name, &where)) {
		//inode *nd = load_inode_by_id(where);
		//return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

		remove_dir_item(parent, where);
		//remove_dir_node_2(nd);
		//free_inode_with_id(where);
	}

	return 0;
}


/****************/
/*		*/
/*    mv	*/
/*		*/
/****************/


int move(int32_t sparent, int32_t tparent, char* sname, char *tname) {
	int32_t tgt_id = tparent, src_id = sparent;

	
	inode *sprnt = load_inode_by_id(sparent);
	return_error_on_condition(!sprnt, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	inode *tprnt = load_inode_by_id(tparent);
	return_error_on_condition(!tprnt, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	if(search_dir(tname, &tgt_id)) {
		printf("ERROR: Failed to find target in parent");
		return 1;
	}

	if(search_dir(sname, &src_id)) {
		printf("ERROR: Failed to find target in parent");
		return 1;
	}


	inode *src = load_inode_by_id(src_id);
	return_error_on_condition(!src, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	inode *tgt = load_inode_by_id(tgt_id);
	return_error_on_condition(!tgt, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	if(src->isDirectory == 1 && tgt->isDirectory != 1) {
		printf("CANNOT MOVE DIR TO FILE\n");
		return 1;
	}

	if(tgt->isDirectory == 1) {
		directory_item *di = extract_dir_item_from_dir(sprnt->nodeid, src->nodeid);
		return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

		append_dir_item(di, tgt);

		free(di);
	} else if(tgt->isDirectory == 0) {
		remove_dir_item(tprnt->nodeid, tgt->nodeid);
		free_inode_with_id(tgt->nodeid);

		directory_item *di = extract_dir_item_from_dir(sprnt->nodeid, src->nodeid);
		return_error_on_condition(!di, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

		bzero(di->item_name, MAX_ITEM_NAME_LENGTH);
		memcpy(di->item_name, tname, MAX_ITEM_NAME_LENGTH - 1);

		append_dir_item(di, tprnt);

		free(di);
	} else {
		printf("ERROR: Unknown item\n");
		return 1;
	}

	free(sprnt);
	free(tprnt);
	free(src);
	free(tgt);
	
	return 0;
}

/****************/
/*		*/
/*    cp	*/
/*		*/
/****************/


int copy_file(inode *source, inode *target) {
	int rv = 0, it = 1;
	int block_count = source->file_size / sblock->cluster_size + 1;
	char buffer[sblock->cluster_size + 1];
	
	buffer[sblock->cluster_size] = 0x00;

	free_allocated_blocks(target); // "remove" data of the previous file

	if(allocate_blocks_for_file(target, block_count)) {
		return 1;
	}

	target->file_size = source->file_size;

	while(!rv) {
		bzero(buffer, sblock->cluster_size + 1);

		rv = read_data_to_buffer(buffer, source, it);
		if(rv) break; //i spent 2-3 hours to figure this out wtf

		write_data_to_block(buffer, target, it);
		it++;
	}

	
	return 0;
}


int copy(int32_t sparent, int32_t tparent, char* sname, char *tname) {
	int32_t tgt_id = tparent, src_id = sparent;
	int is_new = 0;
	inode *tgt, *new = NULL;

	
	inode *sprnt = load_inode_by_id(sparent);
	return_error_on_condition(!sprnt, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	inode *tprnt = load_inode_by_id(tparent);
	return_error_on_condition(!tprnt, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

//loading source
	if(search_dir(sname, &src_id)) {
		printf("FILE NOT FOUND");
		return 1;
	}

	inode *src = load_inode_by_id(src_id);
	return_error_on_condition(!src, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	if(src->isDirectory) {
		printf("CANNOT COPY DIRECTORY\n");
		return 1;
	}

	//int block_count = src->file_size / sizeof(sblock->cluster_size) + 1;
//end loading source

//loading target
	if(search_dir(tname, &tgt_id)) {
		tgt = prepare_new_file_node();

		if(!tgt) {
			//printf("ERROR: Failed to create target. Out of inodes.");
			free(sprnt);
			free(tprnt);
			free(src);

			return 2;
		}

		is_new = 1;
	} else {
		tgt = load_inode_by_id(tgt_id);
		return_error_on_condition(!tgt, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);
	}
//end loading target

	if(tgt->isDirectory == 1) {
		new = prepare_new_file_node();

		if(!new) {
			//printf("ERROR: Failed to create target. Out of inodes.");
			free(sprnt);
			free(tprnt);
			free(src);
			free(tgt);

			return 2;
		}

		if(copy_file(src, new)) {
			printf("ERROR COPYING FILE\n");
		}

		directory_item *di = calloc(1, sizeof(directory_item));
		di->inode = new->nodeid;
		memcpy(di->item_name, sname, MAX_ITEM_NAME_LENGTH - 1);

		append_dir_item(di, tgt);
		save_inode(new);
	} else if(tgt->isDirectory == 0) {
		if(copy_file(src, tgt)) {
			printf("ERROR COPYING FILE\n");
		}

		if(is_new) {
			directory_item *di = calloc(1, sizeof(directory_item));
			di->inode = tgt->nodeid;
			memcpy(di->item_name, tname, MAX_ITEM_NAME_LENGTH - 1);
	
			append_dir_item(di, tprnt);
			save_inode(tgt);
		}
	} else {
		printf("ERROR: Unknown item\n");
		return 1;
	}

	free(sprnt);
	free(tprnt);
	free(src);
	free(tgt);


	return 0;
}

/****************/
/*		*/
/*    info	*/
/*		*/
/****************/


int node_info(int32_t where, char *name) {
	int node_id = where;

	inode *parent = load_inode_by_id(where);
	return_error_on_condition(!parent, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

	if(search_dir(name, &node_id)) {
		printf("ERROR: Failed to find %s\n", name);
		return 1;
	} else {
		inode *node = load_inode_by_id(node_id);
		return_error_on_condition(!node, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

		printf("NAME\tSIZE\tINODE\tREFERENCES\n");
		printf("%s\t%d\t%d\t%d\n", name, node->file_size, node->nodeid, node->references);
	}

	return 0;
}

/****************/
/*		*/
/*    outcp	*/
/*		*/
/****************/


int out_copy(int32_t where, char *name, char *target) {
	FILE *f = fopen(target, "wb");

	if(!f) {
		printf("FAILED TO OPEN/ CREATE OUTPUT FILE");
	}

	int rv = 0, it = 1, write = 0;
	char buffer[sblock->cluster_size + 1];
	
	buffer[sblock->cluster_size] = 0x00;

	if(!search_dir(name, &where)) {
		inode *nd = load_inode_by_id(where);
		return_error_on_condition(!nd, MEMORY_ALLOCATION_ERROR_MESSAGE, 1);

		if(nd->isDirectory == 1) {
			printf("CANNOT OUTPUT DIRECTORY");
			fclose(f);
			return 1;
		}

		while(!rv) {
			rv = read_data_to_buffer(buffer, nd, it);
			if(rv) break;

			if((write = nd->file_size - (it - 1) * sblock->cluster_size) > sblock->cluster_size) {
				write = sblock->cluster_size;
			}

			printf("Writing %d\n", write);

			fwrite(buffer, sizeof(char), write, f);
			it++;
		}
	} else {
		fclose(f);
		//printf("Failed to find %s in dir\n", name);
		return 1;
	}

	fclose(f);
	
	return 0;
}
