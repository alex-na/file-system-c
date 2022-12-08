// Directory manipulation functions.
//
// Feel free to use as inspiration.

// based on cs3650 starter code
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "blocks.h"
#include "inode.h"
#include "slist.h"
#include "directory.h"
#include "bitmap.h"

int MAX_DIR_ENTRIES = 4096 / sizeof(dirent_t);

// Assuming root directory is at inode 2, initialize the entries of the root directory
// Allocate inumber 2 for root, and the first available block for the root node!
void directory_init() {
	int bnum = alloc_block();
	assert(bnum != -1);
	inode_t * root_node = get_inode(2);
	root_node->size = 1;
	root_node->mode = 0;
	root_node->block = bnum;
	root_node->refs = 1;
	dirent_t * root_block_entries = (dirent_t *)blocks_get_block(bnum);
	strcpy(root_block_entries[0].name, ".");
	root_block_entries[0].inum = 2;
}

// Returns the inumber of the file with the given name that matches one
// of the entries in the directory's data block.
// If no match is found, return -1. 
int directory_lookup(inode_t *dd, const char *name){
  int bnum = dd->block;
  int num_files = dd->size;
  dirent_t * block = (dirent_t *) blocks_get_block(bnum);
  for (int i = 0; i < num_files; ++ i) {
    if (strcmp(block[i].name, name) == 0) {
			return block[i].inum;
		}
  }
	return -1;
}

// Returns the inumber of the file or folder corresponding to the given path.
// If no file is found, return -1.
// If the path was invalid, or one of the supposed directories are not actually directories but a file,
// we also return -1. For example: /foo/hello.txt/bar/hello1.txt
int tree_lookup(const char *path) {
	if (strcmp(path, "/") == 0) return 2;
	slist_t * curr_file_name = s_explode(path, '/');
	inode_t * curr_dir_inode = get_inode(2);
	int curr_file_inum = -1;
	while (curr_file_name) {
		curr_file_inum = directory_lookup(curr_dir_inode, curr_file_name->data);
		if (curr_file_inum == -1) return -1;
		curr_dir_inode = get_inode(curr_file_inum);
		// CHECK IF CURR_FILE_NODE is a file or dir, if file, stop! If dir, keep going. 
		if (curr_dir_inode->mode == 1) return -1;
		curr_file_name = curr_file_name->next;
	}
	return curr_file_inum;
}

int updateParentAndCreate(inode_t * curr_dir_inode, const char * file_name, int isDir) {
	int allocatedBlock = alloc_block();
	int allocatedInum = alloc_inode();
	printf("Block = %d, inode= %d\n", allocatedBlock, allocatedInum);
  if (allocatedBlock != -1 && allocatedInum != -1) {
    directory_put(curr_dir_inode, file_name, allocatedInum);
	  curr_dir_inode->size++;
    inode_t * new_file_node = get_inode(allocatedInum);
    new_file_node->mode = 1;
    new_file_node->refs = 1;
    new_file_node->size = 0;
    new_file_node->block = allocatedBlock;
	  if (isDir) {
			int dir_inum = directory_lookup(curr_dir_inode, ".");
	    curr_dir_inode->refs++;
      new_file_node->mode = 0;
      directory_put(new_file_node, ".", allocatedInum);
      directory_put(new_file_node, "..", dir_inum);
    }
		return 0;
	}
	// RESTORE THE BLOCK AND INODE TO FREE STATE
	return -1;
}

// update the parent, and update the inode of the file/folder, update the bitmaps if the refs of the file/folder is 0.

int updateParentAndRemove(inode_t * curr_dir_inode, const char * file_name) {
	int fileInum = directory_lookup(curr_dir_inode, file_name);
	if (fileInum == -1) return -ENOENT;
	inode_t * fileNode = get_inode(fileInum);	
	fileNode->refs--;
	if (fileNode->refs == 0) {
		void * bbm = get_blocks_bitmap();
		void * ibm = get_inode_bitmap();
		bitmap_put(bbm, fileNode->block, 0);
		bitmap_put(ibm, fileInum, 0);
		return directory_delete(curr_dir_inode, file_name);
	}
	return 0;	
}

// function returns the inumber of the parent of the given file.
// /foo/hello.txt -> / -> foo hello.txt
// FREE THE SLIST_T!!!
int get_parent_inum(const char * path, int removeHuh, int dirHuh) {
	printf("path= %s\n", path); // this prints path correct 
	slist_t * curr_file_name = s_explode(path, '/');
	printf("curr_file_name%s\n", curr_file_name[0].data); // this prints nothing
	// if the file is something like /hello.txt -> the parent is the root.
  inode_t * curr_dir_inode = get_inode(2);
	if (curr_file_name->next = NULL) {
    if (removeHuh) return updateParentAndRemove(curr_dir_inode, curr_file_name->data);
    else return updateParentAndCreate(curr_dir_inode, curr_file_name->data, dirHuh);
  }
  int curr_file_inum = -1;
  while (curr_file_name) {
    curr_file_inum = directory_lookup(curr_dir_inode, curr_file_name->data);
    if (curr_file_inum == -1) break;
    curr_dir_inode = get_inode(curr_file_inum);
    // CHECK IF CURR_FILE_NODE is a file or dir, if file, stop! If dir, keep going. 
    if (curr_dir_inode->mode == 1) break;
		if (curr_file_name->next->next = NULL) {
			if (removeHuh) return updateParentAndRemove(curr_dir_inode, curr_file_name->next->data);
			else return updateParentAndCreate(curr_dir_inode, curr_file_name->next->data, dirHuh);
		}
    curr_file_name = curr_file_name->next;
  }
  return -1;
}

int removeFile(const char * path) {
	return get_parent_inum(path, 1, 0);
}

int createFile(const char * path) {
	printf("creatFile path= %s\n", path);
	return get_parent_inum(path, 0, 0);
}

int createDirectory(const char * path) {
	return get_parent_inum(path, 0, 1); // returns a new inumber for the new dir being created
	// also, updates the parents' entries to contain new dir, and inits the entries of the new dir to contain	// . and ..
}

int directory_put(inode_t *dd, const char *name, int inum) {}
int directory_delete(inode_t *dd, const char *name) {}
slist_t *directory_list(const char *path) {}
void print_directory(inode_t *dd) {}

