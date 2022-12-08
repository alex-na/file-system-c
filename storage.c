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

#include "slist.h"
#include "directory.h"
#include "inode.h"
#include "directory.h"

void storage_init(const char *path) {
  int fd = open(path, O_CREAT | O_RDWR, 0644);
  assert(fd != -1);
  void * mm = mmap(0, 4096 * 256, PROT_READ | PROT_WRITE,  MAP_SHARED, fd, 0);
  assert(mm != (void *) -1);
	blocks_init(path);
}

int storage_stat(const char *path, struct stat *st) {}

int storage_read(const char *path, char *buf, size_t size, off_t offset) {
  int fileInum = tree_lookup(path);
  if (fileInum == -1) return -1;
  assert(size > 0 && offset >= 0);
  inode_t * fileInode = get_inode(fileInum);
  assert(fileInode != NULL);
  void * blk = blocks_get_block(fileInode->block);
  if (fileInode->size < size + offset) return -1;
  memcpy(buf, blk + offset, size);
  return 0;

}
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {}
int storage_truncate(const char *path, off_t size) {}
int storage_mknod(const char *path, int mode) {}
int storage_unlink(const char *path) {}
int storage_link(const char *from, const char *to) {}
int storage_rename(const char *from, const char *to) {}
int storage_set_time(const char *path, const struct timespec ts[2]) {}
slist_t *storage_list(const char *path) {}

