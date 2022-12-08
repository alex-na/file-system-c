// Inode manipulation routines.
//
// Feel free to use as inspiration.

// based on cs3650 starter code
#include <assert.h>

#include "blocks.h"
#include "inode.h"
#include "bitmap.h"

int NINODES = 4096 / sizeof(inode_t);

void print_inode(inode_t *node) {
  assert(node != NULL);
  printf("Reference count:%d\n", node->refs);
  printf("Access mode:%d\n", node->mode);
  printf("Size:%d\n", node->size);
  printf("In block nb:%d\n", node->block);
}

// PLEASE RECHECK IF THIS IS CORRECT!!!
inode_t *get_inode(int inum) {
	inode_t * inode_table = (inode_t*)get_blocks_inode_table();
	return &inode_table[inum];	
}

// should this return a free inode from the inode table??
int alloc_inode() {
  void* ibm = get_inode_bitmap();
  // for each bit in the array of bits
  for (int i = 0; i < NINODES;++i) {
     int status = bitmap_get(ibm, i);
     if (!status) {
       bitmap_put(ibm, i, 1);	     
       return i;
     }
  }
  return -1;
}

// WHICH INODE ARE WE FREEING???
void free_inode(int inum) {
  void* ibm = get_inode_bitmap();
  bitmap_put(ibm, inum, 0);
}

// what is the return supposed to be? Maybe inum?
// Is it supposed to increment the node's size with size, or set it to size???!
int grow_inode(inode_t *node, int size) {
  node->size = size;
  return 0;
}

// same questions as above
int shrink_inode(inode_t *node, int size){
  node->size = size;
  return 0;
}

// WHAT IS THIS SUPPOSED TO DO???
int inode_get_bnum(inode_t *node, int fbnum) {
  void * ibm = get_inode_bitmap();
}
