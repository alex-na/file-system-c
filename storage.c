// Disk storage manipulation.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "slist.h"
#include "directory.h"
#include "inode.h"
#include "directory.h"

void storage_init(const char *path) {
	blocks_init(path);
}

int storage_stat(const char *path, struct stat *st) {}

int storage_read(const char *path, char *buf, size_t size, off_t offset) {
  int fileInum = tree_lookup(path);
  if (fileInum == -1) return -ENOENT;
  assert(size > 0 && offset >= 0 && size <= BLOCK_SIZE);
  inode_t * fileInode = get_inode(fileInum);
  assert(fileInode != NULL);
  void * blk = blocks_get_block(fileInode->block);
	// fileInode's size = 4, size = 4096, offset = 0
	if (size + offset > fileInode->size) size = fileInode->size - offset;
  memcpy(buf, blk + offset, size);
  return size - offset;
}

int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
	int fileInum = tree_lookup(path);
	if (fileInum == -1) return -ENOENT;
	assert(size > 0 && offset >= 0 && size <= strlen(buf));
	inode_t * fileInode = get_inode(fileInum);
	assert(fileInode != NULL);
	if (BLOCK_SIZE - (fileInode->size + size - offset) < 0) {
		printf("No space in file\n");
		return 0;
	}
	void * blk = blocks_get_block(fileInode->block);
  memcpy(blk + offset, buf, size);
	fileInode->size += size;
  return size;
}

int storage_truncate(const char *path, off_t size) {
	int fileInum = tree_lookup(path);
	if (fileInum == -1) return -1;
	inode_t * fileNode = get_inode(fileInum);
	fileNode->size -= size;
	return 0;
}

int storage_mknod(const char *path, int mode) {
	printf("IN STORAGE_MKNOd\n"); 
	if (S_ISDIR(mode)) {
		return createDirectory(path);
	}
	else {
		return createFile(path);	
	}
}

// PREFERABLE, HAVE A GLOBAL VAR FOR ROOT INODE
// THROW THE RIGHT ERROR CODES
int storage_unlink(const char *path) {
	return removeFile(path);
}
int storage_link(const char *from, const char *to) {}

int storage_rename(const char *from, const char *to) {
	return renameHelper(from, to);
}

int storage_set_time(const char *path, const struct timespec ts[2]) {}
slist_t *storage_list(const char *path) {}

