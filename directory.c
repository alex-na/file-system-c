// Directory manipulation functions.
//
// Feel free to use as inspiration.

// based on cs3650 starter code
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "bitmap.h"
#include "blocks.h"
#include "directory.h"
#include "inode.h"
#include "slist.h"

int MAX_DIR_ENTRIES = 4096 / sizeof(dirent_t);

// Assuming root directory is at inode 2, initialize the entries of the root
// directory Allocate inumber 2 for root, and the first available block for the
// root node.
void directory_init() {
  int bnum = alloc_block();
  assert(bnum != -1);
  inode_t *root_node = get_inode(2);
  // setting inode fields of root directory
  root_node->size = 1;
  root_node->mode = 0;
  root_node->block = bnum;
  root_node->refs = 1;
  dirent_t *root_block_entries = (dirent_t *)blocks_get_block(bnum);
  // adding self reference
  char root[2] = ".";
  memcpy(root_block_entries[0].name, root, 2);
  root_block_entries[0].inum = 2;
}

// Returns the inumber of the file with the given name that matches one
// of the entries in the directory's data block.
// If no match is found, return -1.
int directory_lookup(inode_t *dd, const char *name) {
  int bnum = dd->block;
  int num_files = dd->size;
  dirent_t *block = (dirent_t *)blocks_get_block(bnum);
  for (int i = 0; i < num_files; ++i) {
    if (strcmp(block[i].name, name) == 0) {
      return block[i].inum;
    }
  }
  return -1;
}

// Returns the inumber of the file or directory corresponding to the given path.
// If no file is found, return -1.
// If the path was invalid, or one of the supposed directories are not actually
// directories but a file, we also return -1. For example:
// /foo/hello.txt/bar/hello1.txt
int tree_lookup(const char *path) {
  // if root dir, return 2
  if (strcmp(path, "/") == 0)
    return 2;
  slist_t *curr_file_name = s_explode(path, '/');
  if (strlen(curr_file_name->data) <= 1)
    curr_file_name = curr_file_name->next;
  // starting from the root directory, perform lookup on each string delimited by "/"
  inode_t *curr_dir_inode = get_inode(2);
  int curr_file_inum = -1;
  while (curr_file_name) {
    curr_file_inum = directory_lookup(curr_dir_inode, curr_file_name->data);
    if (curr_file_inum == -1)
      return -1;
    curr_dir_inode = get_inode(curr_file_inum);
    // CHECK IF CURR_FILE_NODE is a file or dir, if file, stop! If dir, keep going.
    if (curr_dir_inode->mode == 1 && curr_file_name->next != NULL)
      return -1;
    curr_file_name = curr_file_name->next;
  }
  return curr_file_inum;
}

// helper to get final string from a filepath
slist_t *getTarget(const char *path) {
  slist_t *curr = s_explode(path, '/');
  while (curr->next) {
    curr = curr->next;
  }
  return curr;
}

// helper to get parent of the final string in file path
int getParent(const char *path) {
  slist_t *curr_file = s_explode(path, '/'), *prev = NULL;
  if (strlen(curr_file->data) <= 1)
    curr_file = curr_file->next;
  if (curr_file->next == NULL)
    return 2;
  // starting from root directory, perform lookups on each string until last string is hit
  // when hit, return the inumber of its parent
  inode_t *curr_dir = get_inode(2);
  int curr_file_inum = -1;
  while (curr_file->next) {
    curr_file_inum = directory_lookup(curr_dir, curr_file->data);
    if (curr_file_inum == -1)
      return -1;
    curr_dir = get_inode(curr_file_inum);
    curr_file = curr_file->next;
  }
  return curr_file_inum;
}

// helper to update the parent of the final string in filepath and initializing
// childs data in the parent
int updateParentAndCreate(inode_t *curr_dir_inode, const char *file_name,
                          int isDir, int block, int inum, int size) {
  // allocate new block and inode
  int allocatedBlock = block;
  int allocatedInum = inum;
  int targetSize = size;
  if (block == -1)
    allocatedBlock = alloc_block();
  if (inum == -1)
    allocatedInum = alloc_inode();
  if (size == -1)
    targetSize = 0;
  // create new entry in parent directory and udpate the new inode data
  if (allocatedBlock != -1 && allocatedInum != -1) {
    directory_put(curr_dir_inode, file_name, allocatedInum);
    inode_t *new_file_node = get_inode(allocatedInum);
    new_file_node->mode = 1;
    new_file_node->refs = 1;
    new_file_node->size = targetSize;
    new_file_node->block = allocatedBlock;
	// if we are creating a directory, update parents and self refernces 
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

// helper for renaming a file/directory
int renameHelper(const char *from, const char *to) {
  // get the parent's inum from the "from" filepath
  int fromParentInum = getParent(from);
  if (fromParentInum == -1)
    return -ENOENT;
  inode_t *fromParentNode = get_inode(fromParentInum);
  slist_t *fromTarget = getTarget(from);
  // lookup the final string in "from" filepath within its parent
  int fromInum = directory_lookup(get_inode(fromParentInum), fromTarget->data);
  // update parent and remove the target string from parent
  if (updateParentAndRemove(fromParentNode, fromTarget->data, 1) == -1)
    return -ENOENT;
  if (fromInum == -1)
    return -ENOENT;
  inode_t *fromNode = get_inode(fromInum);
  int fileType = 0;
  if (fromNode->mode == 0)
    fileType = 1;
  // get the parent of the location that we are moving to
  int toParentInum = getParent(to);
  if (toParentInum == -1)
    return -ENOENT;
  inode_t *parentInode = get_inode(toParentInum);
  slist_t *target = getTarget(to);
  // update the parent in the "to" filepath to now include the moved files data
  int rv = updateParentAndCreate(parentInode, target->data, fileType,
                                 fromNode->block, fromInum, fromNode->size);
  s_free(target);
  return rv;
}

// update the parent, and update the inode of the file/folder, update the
// bitmaps if the refs of the file/folder is 0.
int updateParentAndRemove(inode_t *curr_dir_inode, const char *file_name,
                          int renameHuh) {
  // perform lookup on the filepath 
  int fileInum = directory_lookup(curr_dir_inode, file_name);
  if (fileInum == -1)
    return -ENOENT;
  inode_t *fileNode = get_inode(fileInum);
  fileNode->refs--;
  // if file has 0 refs, free its data and inode
  if (fileNode->refs == 0) {
    if (!renameHuh) {
      free_block(fileNode->block);
      free_inode(fileInum);
    }
    // If the file to be removed is a dir, decrement parent references
    if (fileNode->mode == 0)
      curr_dir_inode->refs--;
	// delete file from its parent directory
    return directory_delete(curr_dir_inode, file_name);
  }
  return 0;
}

// multi-purpose helper function for dealing with parent data.
// function returns the inumber of the parent of the given file.
// adds/removes based on removeHuh flag, renames based on renameHuh flag
// and works with files/directories based on dirHuh flag.
int get_parent_inum(const char *path, int removeHuh, int renameHuh,
                    int dirHuh) {
  slist_t *curr_file_name = s_explode(path, '/');
  inode_t *curr_dir_inode = get_inode(2);
  if (strlen(curr_file_name->data) <= 1)
    curr_file_name = curr_file_name->next;
  if (curr_file_name->next == NULL) {
	// if renaming, update parent and remove child 
    if (renameHuh) {
      if (updateParentAndRemove(curr_dir_inode, curr_file_name->data, renameHuh) == 0) {
	    // perform lookup on parent to return its inum
        return directory_lookup(curr_dir_inode, curr_file_name->data);
      } else
        return -1; // return -1 if updateParentAndRemove fails
	// if removing, update parent and remove its child
    } else if (removeHuh)
      return updateParentAndRemove(curr_dir_inode, curr_file_name->data, renameHuh);
	// else, update the parent and create child
    else
      return updateParentAndCreate(curr_dir_inode, curr_file_name->data, dirHuh, -1, -1, -1);
  }
  int curr_file_inum = -1;
  // starting at the root, perform lookups until we reach the parent of the final string in path
  while (curr_file_name->next) {
    curr_file_inum = directory_lookup(curr_dir_inode, curr_file_name->data);
    if (curr_file_inum == -1)
      break;
    curr_dir_inode = get_inode(curr_file_inum);
    if (curr_file_name->next->next == NULL) {
	  // if renaming, update parent and remove child 
      if (renameHuh) {
        if (updateParentAndRemove(curr_dir_inode, curr_file_name->data, renameHuh) == 0) {
		  // perform lookup on parent to return its inum
          return directory_lookup(curr_dir_inode, curr_file_name->data);
        } else
          return -1;
	  // if removing, update parent and remove its child
      } else if (removeHuh)
        return updateParentAndRemove(curr_dir_inode, curr_file_name->next->data, renameHuh);
	  // else, update the parent and create child
      else
        return updateParentAndCreate(curr_dir_inode, curr_file_name->next->data, dirHuh, -1, -1, -1);
    }
    curr_file_name = curr_file_name->next;
  }
  return -1;
}

// removing a file
int removeFile(const char *path) { return get_parent_inum(path, 1, 0, 0); }

// creating a file
int createFile(const char *path) {
  void *ibm = get_inode_bitmap();
  void *bbm = get_blocks_bitmap();
  bitmap_print(ibm, 256);
  bitmap_print(bbm, 256);
  // returns inumber of file being created
  return get_parent_inum(path, 0, 0, 0);
}

// creating a directory
int createDirectory(const char *path) {
  // returns a new inumber for the new dir being created
  return get_parent_inum(path, 0, 0, 1); 
}

// adding an entry to a directory
int directory_put(inode_t *dd, const char *name, int inum) {
  if (dd->mode == 1) {
    return -1;
  }
  if (dd->size == MAX_DIR_ENTRIES)
    return -1;
  dirent_t *blk_dir = (dirent_t *)blocks_get_block(dd->block);
  blk_dir[dd->size].inum = inum;
  strcpy(blk_dir[dd->size].name, name);
  dd->size++;
  return 0;
}

// replace the last entry in directory with an empty directory for easier
// indexing
void directory_replace_last(inode_t *dd, int fromIndex) {
  dirent_t *dir_entries = (dirent_t *)blocks_get_block(dd->block);
  strcpy(dir_entries[fromIndex].name, dir_entries[dd->size - 1].name);
  dir_entries[fromIndex].inum = dir_entries[dd->size - 1].inum;
  dir_entries[dd->size - 1].inum = -1;
}

// Delete the file with the given name from the directory entries of the given
// inode. If no name was found, then -1 is returned.
int directory_delete(inode_t *dd, const char *name) {
  dirent_t *curr_dir_entries = (dirent_t *)blocks_get_block(dd->block);
  for (int i = 0; i < dd->size; ++i) {
    if (strcmp(curr_dir_entries[i].name, name) == 0) {
      directory_replace_last(dd, i);
      dd->size--;
      return 0;
    }
  }
  return -1;
}
