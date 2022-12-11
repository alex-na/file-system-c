// Inode manipulation routines.
//
// Feel free to use as inspiration.

// based on cs3650 starter code
#include <assert.h>

#include "bitmap.h"
#include "blocks.h"
#include "inode.h"

int NINODES = 4096 / sizeof(inode_t);

// print an inodes data
void print_inode(inode_t *node) {
  assert(node != NULL);
  printf("Reference count:%d\n", node->refs);
  printf("Access mode:%d\n", node->mode);
  printf("Size:%d\n", node->size);
  printf("In block nb:%d\n", node->block);
}

// return an inode based on an inumber
inode_t *get_inode(int inum) {
  inode_t *inode_table = (inode_t *)get_blocks_inode_table();
  return &inode_table[inum];
}

// allocating an inode
int alloc_inode() {
  void *ibm = get_inode_bitmap();
  // for each bit in the array of bits
  for (int i = 0; i < NINODES; ++i) {
    int status = bitmap_get(ibm, i);
    if (!status) {
      bitmap_put(ibm, i, 1);
      return i;
    }
  }
  return -1;
}
