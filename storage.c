// Disk storage manipulation.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#include "directory.h"
#include "inode.h"
#include "slist.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// initialzing our storage by calling blocks_init()
void storage_init(const char *path) { blocks_init(path); }

// reading a file
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
  // perform lookup on the filepath to check whether it exists
  int fileInum = tree_lookup(path);
  if (fileInum == -1)
    return -ENOENT; // throw error if not found
  assert(size > 0 && offset >= 0 && size <= BLOCK_SIZE);
  inode_t *fileInode = get_inode(fileInum);
  assert(fileInode != NULL);
  // get the block from the inode and read its contents to buffer based on size/offset
  void *blk = blocks_get_block(fileInode->block);
  if (size + offset > fileInode->size)
    size = fileInode->size - offset;
  memcpy(buf, blk + offset, size);
  return size - offset;
}

// writing to a file
int storage_write(const char *path, const char *buf, size_t size,
                  off_t offset) {
  // perform lookup on the filepath to check whether it exists
  int fileInum = tree_lookup(path);
  if (fileInum == -1)
    return -ENOENT; // throw error if not found
  assert(size > 0 && offset >= 0 && size <= strlen(buf));
  inode_t *fileInode = get_inode(fileInum);
  assert(fileInode != NULL);
  // checking whether there is enough space to write to the file based on the size/offset
  if (BLOCK_SIZE - (fileInode->size + size - offset) < 0) {
    printf("No space in file\n");
    return 0;
  }
  // get the block from the inode and write to the file from the buffers contents
  void *blk = blocks_get_block(fileInode->block);
  memcpy(blk + offset, buf, size);
  fileInode->size += size;
  return size;
}

// truncating a file
int storage_truncate(const char *path, off_t size) {
  int fileInum = tree_lookup(path);
  if (fileInum == -1)
    return -1;
  // decrement the size of the file based on the given size
  inode_t *fileNode = get_inode(fileInum);
  fileNode->size -= size;
  return 0;
}

// opening a file/directory
int storage_mknod(const char *path, int mode) {
  printf("IN STORAGE_MKNOd\n");
  if (S_ISDIR(mode)) {
    return createDirectory(path);
  } else {
    return createFile(path);
  }
}

// unlinking a file/directory
int storage_unlink(const char *path) { return removeFile(path); }

// renaming a file/directory
int storage_rename(const char *from, const char *to) {
  return renameHelper(from, to);
}
