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
	char root[2] = ".";
	memcpy(root_block_entries[0].name, root, 2);
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
		printf("Looking for: %s, curr: %s\n", name, block[i].name);
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
	if (strlen(curr_file_name->data) <= 1) curr_file_name = curr_file_name->next;
	inode_t * curr_dir_inode = get_inode(2);
	int curr_file_inum = -1;
	while (curr_file_name) {
		curr_file_inum = directory_lookup(curr_dir_inode, curr_file_name->data);
		if (curr_file_inum == -1) return -1;
		curr_dir_inode = get_inode(curr_file_inum);
		// CHECK IF CURR_FILE_NODE is a file or dir, if file, stop! If dir, keep going. 
		if (curr_dir_inode->mode == 1 && curr_file_name->next != NULL) return -1;
		curr_file_name = curr_file_name->next;
	}
	return curr_file_inum;
}

slist_t * getTarget(const char * path) {
  slist_t * curr = s_explode(path, '/');
  while (curr->next) {
    curr = curr->next;
  }
  return curr;
}

int getParent(const char * path) {
  slist_t * curr_file = s_explode(path, '/'), * prev = NULL;
  if (strlen(curr_file->data) <= 1) curr_file = curr_file->next;
  if (curr_file->next == NULL) return 2;
  inode_t * curr_dir = get_inode(2);
	int curr_file_inum = -1;
  while (curr_file->next) {
    curr_file_inum = directory_lookup(curr_dir, curr_file->data);
		printf("curr_dir's inum: %d\n", curr_file_inum);
		if (curr_file_inum == -1) return -1;
    curr_dir = get_inode(curr_file_inum);
    curr_file = curr_file->next;
  }
  return curr_file_inum;
}

int updateParentAndCreate(inode_t * curr_dir_inode, const char * file_name, int isDir, int block, int inum, int size) {
	int allocatedBlock = block;
	int allocatedInum = inum; 
	int targetSize = size;
	if (block == -1) allocatedBlock = alloc_block();
  if (inum == -1) allocatedInum = alloc_inode();
	if (size == -1) targetSize = 0;
  if (allocatedBlock != -1 && allocatedInum != -1) {
    directory_put(curr_dir_inode, file_name, allocatedInum);
//	  curr_dir_inode->size++;
    inode_t * new_file_node = get_inode(allocatedInum);
    new_file_node->mode = 1;
    new_file_node->refs = 1;
    new_file_node->size = targetSize;
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

// RENAMING:
// from = /foo/bar/hello.txt -> to = /hello1.txt
// IN THE CASE OF A FILE:
// get the inum of hello.txt
// get the inode, and store the block
// delete from the parent 
// Get the parent directory of to
// Add new file to the parent with the inum of hello.txt
// Increment the size of the parent

// IN THE CASE OF A DIR:
// from = /foo/bar/foo1 -> to /foo2
// get the inum of foo1
// get the inode, and store the block
// delete from the parent
// get the parent of the to
// add new dir to the parent with the inum of foo1
// director_replace() -> replace the ".."'s inum to be the parent of the to's inum

int renameHelper(const char * from, const char * to) {
	printf("from: %s, to: %s\n", from, to);
//	int fromInum = get_parent_inum(from, 0, 1, 0);
	int fromParentInum = getParent(from);
	if (fromParentInum == -1) return -ENOENT;
	inode_t * fromParentNode = get_inode(fromParentInum);
	slist_t * fromTarget = getTarget(from);
	printf("Directory lookup!!\n");
	int fromInum = directory_lookup(get_inode(fromParentInum), fromTarget->data);
	printf("Removing!!!\n");
	if (updateParentAndRemove(fromParentNode, fromTarget->data, 1) == -1) return -ENOENT;
	printf("From inum : %d\n", fromInum);
	if (fromInum == -1) return -ENOENT;
	inode_t * fromNode = get_inode(fromInum);
	int fileType = 0;
	if (fromNode->mode == 0) fileType = 1;
	int toParentInum = getParent(to);
	printf("From inum = %d, toParentInum = %d\n", fromInum, toParentInum);
	if (toParentInum == -1) return -ENOENT;
	inode_t * parentInode= get_inode(toParentInum);
	slist_t * target = getTarget(to);
	printf("Target: %s\n", target->data);
	int rv = updateParentAndCreate(parentInode, target->data, fileType, fromNode->block, fromInum, fromNode->size);
	s_free(target);
	return rv;
}

// update the parent, and update the inode of the file/folder, update the bitmaps if the refs of the file/folder is 0.
int updateParentAndRemove(inode_t * curr_dir_inode, const char * file_name, int renameHuh) {
	int fileInum = directory_lookup(curr_dir_inode, file_name);
	if (fileInum == -1) return -ENOENT;
	inode_t * fileNode = get_inode(fileInum);	
	fileNode->refs--;
	if (fileNode->refs == 0) {
		if (!renameHuh) {
			free_block(fileNode->block);
      free_inode(fileInum);
		}
		// If the file to be removed is a dir, decrement parent references!
		if (fileNode->mode == 0) curr_dir_inode->refs--;
		return directory_delete(curr_dir_inode, file_name);
	}
	return 0;	
}

// function returns the inumber of the parent of the given file.
// /foo/hello.txt -> / -> foo hello.txt
// FREE THE SLIST_T!!!
// IDEA FOR LATER: COMBINE REMOVEHUH AND RENAMEHUH FLAG = 0 1 2
int get_parent_inum(const char * path, int removeHuh, int renameHuh, int dirHuh) {
	slist_t * curr_file_name = s_explode(path, '/');
	// if the file is something like /hello.txt -> the parent is the root.
  inode_t * curr_dir_inode = get_inode(2);
	if (strlen(curr_file_name->data) <= 1) curr_file_name = curr_file_name->next;
	if (curr_file_name->next == NULL) {
		if (renameHuh) {
			printf("If 1\n");
			if (updateParentAndRemove(curr_dir_inode, curr_file_name->data, renameHuh) == 0) {
				printf("If 2\n");
				return directory_lookup(curr_dir_inode, curr_file_name->data);
			}
			else return -1;
		}
    else if (removeHuh) return updateParentAndRemove(curr_dir_inode, curr_file_name->data, renameHuh);
    else return updateParentAndCreate(curr_dir_inode, curr_file_name->data, dirHuh, -1, -1, -1);
  }
  int curr_file_inum = -1;
  while (curr_file_name->next) { // dirdir2 in dirdir 
    curr_file_inum = directory_lookup(curr_dir_inode, curr_file_name->data);
    if (curr_file_inum == -1) break;
    curr_dir_inode = get_inode(curr_file_inum);
    // CHECK IF CURR_FILE_NODE is a file or dir, if file, stop! If dir, keep going. 
    if (curr_dir_inode->mode == 1) break;
		if (curr_file_name->next->next == NULL) {
			if (renameHuh) {
      	if (updateParentAndRemove(curr_dir_inode, curr_file_name->data, renameHuh) == 0) {
        	return directory_lookup(curr_dir_inode, curr_file_name->data);
				}
				else return -1;
      }
			else if (removeHuh) return updateParentAndRemove(curr_dir_inode, curr_file_name->next->data, renameHuh);
			else return updateParentAndCreate(curr_dir_inode, curr_file_name->next->data, dirHuh,-1, -1, -1);
		}
    curr_file_name = curr_file_name->next;
  }
  return -1;
}

int removeFile(const char * path) {
	return get_parent_inum(path, 1, 0, 0);
}

int createFile(const char * path) { 
	return get_parent_inum(path, 0, 0, 0);
}

int createDirectory(const char * path) {
	return get_parent_inum(path, 0, 0, 1); // returns a new inumber for the new dir being created
}

// ITERATE DON'T JUMP
int directory_put(inode_t *dd, const char *name, int inum) {
	if (dd->mode == 1) {
		printf("given node is not a directory\n");
		return -1;
	}
	if (dd->size == MAX_DIR_ENTRIES) return -1;
	dirent_t * blk_dir = (dirent_t *) blocks_get_block(dd->block);
	blk_dir[dd->size].inum = inum;
	strcpy(blk_dir[dd->size].name, name);
	dd->size++;
	return 0;
}

void directory_replace_last(inode_t * dd, int fromIndex) {
	dirent_t * dir_entries = (dirent_t *) blocks_get_block(dd->block);
	strcpy(dir_entries[fromIndex].name, dir_entries[dd->size - 1].name);
	dir_entries[fromIndex].inum = dir_entries[dd->size - 1].inum;
	dir_entries[dd->size - 1].inum = -1;
}

// Delete the file with the given name from the directory entries of the given inode.
// If no name was found, then -1 is returned.
int directory_delete(inode_t *dd, const char *name) {
	dirent_t * curr_dir_entries = (dirent_t *)blocks_get_block(dd->block);
	for (int i = 0; i < dd->size; ++i) {
		if (strcmp(curr_dir_entries[i].name, name) == 0) {
			directory_replace_last(dd, i);
			dd->size--;
			return 0;
		}
	}
	return -1;
}

slist_t *directory_list(const char *path) {}
void print_directory(inode_t *dd) {}

