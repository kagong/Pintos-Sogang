#include "vm/swap.h"
#include <stdio.h>
#include <debug.h>
#include <bitmap.h>
#include <stdbool.h>
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "devices/block.h"
#define PG_PER_SECTOR PGSIZE / BLOCK_SECTOR_SIZE
struct block* swap_block;
struct bitmap *swap_bitmap;
void swap_init(){
	swap_block = block_get_role(BLOCK_SWAP);
	swap_bitmap = bitmap_create((block_size(swap_block)*BLOCK_SECTOR_SIZE)/PGSIZE);
	ASSERT(swap_block != NULL || swap_bitmap != NULL);
}
void swap_in(void *upage,size_t idx){
	int i ;
	ASSERT(upage != NULL);
	for(i=idx*(PG_PER_SECTOR);i<(idx+1)*(PG_PER_SECTOR);i++){
		block_read(swap_block,i,upage+(i-idx*PG_PER_SECTOR)*BLOCK_SECTOR_SIZE);
	}
	bitmap_reset(swap_bitmap,idx);
}
int swap_out(void *upage){
	size_t idx=0,i;
	ASSERT(upage != NULL);
	idx = bitmap_scan_and_flip(swap_bitmap,0,1,false);
	if(idx == BITMAP_ERROR)
		printf("error\n");
	for(i=idx*(PGSIZE/BLOCK_SECTOR_SIZE);i<(idx+1)*(PGSIZE/BLOCK_SECTOR_SIZE);i++){

		block_write(swap_block,i,upage+(i-idx*PG_PER_SECTOR)*BLOCK_SECTOR_SIZE);
	}
	return (int)idx;
}
