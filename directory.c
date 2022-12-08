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

int directory_put(inode_t *dd, const char *name, int inum) {}
int directory_delete(inode_t *dd, const char *name) {}
slist_t *directory_list(const char *path) {}
void print_directory(inode_t *dd) {}

