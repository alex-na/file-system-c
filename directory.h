// Directory manipulation functions.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 28

#include "blocks.h"
#include "inode.h"
#include "slist.h"

typedef struct dirent {
  char name[DIR_NAME_LENGTH];
  int inum;
} dirent_t;

void directory_init();
int directory_lookup(inode_t *dd, const char *name);
int tree_lookup(const char *path);
int directory_put(inode_t *dd, const char *name, int inum);
int directory_delete(inode_t *dd, const char *name);
slist_t *directory_list(const char *path);
void print_directory(inode_t *dd);
int updateParentAndCreate(inode_t * curr_dir_inode, const char * file_name, int isDir, int block, int inum, int size);
int updateParentAndRemove(inode_t * curr_dir_inode, const char * file_name, int renameHuh);
int get_parent_inum(const char * path, int removeHuh, int renameHuh, int dirHuh);
int renameHelper(const char * from, const char * to);
int createFile(const char * path);
int removeFile(const char * path);
int createDirectory(const char * path);

#endif
