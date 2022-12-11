// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bitmap.h"
#include "blocks.h"
#include "directory.h"
#include "inode.h"
#include "storage.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

// implementation for: man 2 access
// Checks if a file exists.
int nufs_access(const char *path, int mask) {
  int rv = 0;
  // perform lookup through directory to see if file exists
  if (tree_lookup(path) == -1)
    rv = -ENOENT;
  printf("access(%s, %04o) -> %d\n", path, mask, rv);
  return rv;
}

// Gets an object's attributes (type, permissions, size, etc).
// Implementation for: man 2 stat
// This is a crucial function.
int nufs_getattr(const char *path, struct stat *st) {
  int rv = -1;
  int fileInum = tree_lookup(path);
  if (fileInum == -1)
    rv = -ENOENT; // throw error if file not found
  else {
    // populate st struct with inodes data
    inode_t *fileNode = get_inode(fileInum);
    if (fileNode->mode == 0)
      st->st_mode = 040755;
    else
      st->st_mode = 0100644;
    st->st_size = fileNode->size;
    st->st_uid = getuid();
    st->st_ino = fileInum;
    st->st_nlink = fileNode->refs;
    st->st_blksize = 4096;
    rv = 0;
  }
  printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode,
         st->st_size);
  return rv;
}

// helper for readdir
// mimics getattr, but re-purposed for use in readdir
int helper_readdir(inode_t *dd, void *buf, fuse_fill_dir_t filler) {
  dirent_t *dir_entries = (dirent_t *)blocks_get_block(dd->block);
  struct stat st;
  // iteratively set the st struct fields from each entries inodes
  for (int i = 0; i < dd->size; ++i) {
    inode_t *fileNode = get_inode(dir_entries[i].inum);
    assert(fileNode != NULL);
    if (fileNode->mode == 0)
      st.st_mode = 040755;
    else
      st.st_mode = 0100644;
    st.st_size = fileNode->size;
    st.st_uid = getuid();
    st.st_ino = dir_entries[i].inum;
    st.st_nlink = fileNode->refs;
    st.st_blksize = 4096;
    // add data to the filler
    filler(buf, dir_entries[i].name, &st, 0);
  }
  return 0;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
  int rv = -1;
  int dirInum = tree_lookup(path);
  if (dirInum == -1)
    rv = -ENOENT; // throw error if path not found
  inode_t *dirNode = get_inode(dirInum);
  // throw error if directory is NULL, or file was passed instead
  if (dirNode == NULL || dirNode->mode == 1)
    rv = -ENOENT;
  else
    rv = helper_readdir(dirNode, buf, filler);
  printf("readdir(%s) -> %d\n", path, rv);
  return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
  printf("Here!\n");
  int rv = -1;
  rv = storage_mknod(path, mode);
  printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int nufs_mkdir(const char *path, mode_t mode) {
  int rv = nufs_mknod(path, mode | 040000, 0);
  printf("mkdir(%s) -> %d\n", path, rv);
  return rv;
}

// unlinking an object from its parent
int nufs_unlink(const char *path) {
  int rv = storage_unlink(path);
  printf("unlink(%s) -> %d\n", path, rv);
  return rv;
}

// NOT IMPLEMENTED, IGNORE
int nufs_link(const char *from, const char *to) {
  int rv = -1;
  printf("link(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

// removing/unlinking a directory
int nufs_rmdir(const char *path) {
  int rv = storage_unlink(path);
  printf("rmdir(%s) -> %d\n", path, rv);
  return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int nufs_rename(const char *from, const char *to) {
  int rv = storage_rename(from, to);
  printf("rename(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

// NOT IMPLEMENTED, IGNORE
int nufs_chmod(const char *path, mode_t mode) {
  int rv = -1;
  printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

// truncating a file
int nufs_truncate(const char *path, off_t size) {
  int rv = storage_truncate(path, size);
  printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
  return rv;
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible.
// NOT IMPLEMENTED, IGNORE
int nufs_open(const char *path, struct fuse_file_info *fi) {
  int rv = 0;
  printf("open(%s) -> %d\n", path, rv);
  return rv;
}

// Actually read data
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  int rv = storage_read(path, buf, size, offset);
  printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Actually write data
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
  int rv = storage_write(path, buf, size, offset);
  printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Update the timestamps on a file or directory.
int nufs_utimens(const char *path, const struct timespec ts[2]) {
  printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n", path, ts[0].tv_sec,
         ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, 0);
  return -1;
}

// Extended operations
// NOT IMPLEMENTED, IGNORE
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
  int rv = -1;
  printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
  return rv;
}

// initializing operations to be supported by FUSE
void nufs_init_ops(struct fuse_operations *ops) {
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  // ops->create   = nufs_create; // alternative to mknod
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->write = nufs_write;
  ops->utimens = nufs_utimens;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[]) {
  assert(argc > 2 && argc < 6);
  // printf("TODO: mount %s as data file\n", argv[--argc]);
  storage_init(argv[--argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}
