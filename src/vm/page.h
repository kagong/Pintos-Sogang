#ifndef VM_PAGE_H
#define VM_PAGE_H
#include <hash.h>
#include <stdbool.h>>
#include "vm/frame.h"
struct hash_item
{
	void *upage;
	int idx;
	struct hash_elem elem;
};
void user_destructor (struct hash_elem *e, void *aux);
unsigned user_hash_func(const struct hash_elem *e,void *aux);
bool user_hash_less(const struct hash_elem *a,const struct hash_elem *b,void *aux);
void spt_clear_frame_set_idx(struct frame* victim,int idx);
bool is_in_spt(void* fault_addr);
bool spt_swap(void* fault_addr);
void spt_free(void);
bool spt_add_page(void* upage,void* kpage,bool writable);
bool stack_grow(void* fault_addr);
#endif
