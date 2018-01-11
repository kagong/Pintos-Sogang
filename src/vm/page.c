#include "vm/page.h"
#include "vm/swap.h"
#include <stdio.h>
#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#ifdef USERPROG
#include "userprog/pagedir.h"
#endif
void user_destructor (struct hash_elem *e, void *aux){
	struct hash_item *temp = hash_entry(e,struct hash_item,elem);
	free(temp);
}
unsigned user_hash_func(const struct hash_elem *e,void *aux)
{
	void* page = hash_entry(e,struct hash_item,elem)->upage;
	return hash_bytes(&page,sizeof page);
}
bool user_hash_less(const struct hash_elem *a,const struct hash_elem *b,void *aux)
{
	struct hash_item *A, *B;
	A=hash_entry(a,struct hash_item,elem);
	B=hash_entry(b,struct hash_item,elem);
	return A->upage < B->upage;
}
void spt_clear_frame_set_idx(struct frame* victim,int idx){
	struct hash_item new, *temp = NULL;
	ASSERT(victim->upage != NULL);
	new.upage = victim->upage;
	temp = hash_entry(hash_find(&(victim->t->spt),&(new.elem)), struct hash_item,elem);	
	ASSERT(temp != NULL);
	temp -> idx = idx;
}
bool is_in_spt(void* fault_addr){
	struct hash_item new;
	ASSERT(fault_addr != NULL);
	new.upage = pg_round_down(fault_addr);
	return hash_find(&thread_current()->spt,&new.elem) != NULL;	
}
bool spt_swap(void* fault_addr){
	struct hash_item temp, *target=NULL;
	ASSERT(fault_addr != NULL);
	temp.upage = pg_round_down(fault_addr);
	target =  hash_entry(hash_find(&thread_current()->spt,&temp.elem), struct hash_item,elem);
	if(target -> idx == -1 ||  add_frame(target->upage,NULL,true) == NULL )
		return false;
	swap_in(target->upage,target->idx);
	target -> idx = -1;
	return true;
}
void spt_free(){
	hash_clear(&thread_current()->spt,user_destructor);	
}
bool spt_add_page(void* upage,void* kpage, bool writable){
	struct hash_item *new = NULL, *prev = NULL;
	struct hash_elem *he = NULL;
	struct frame *temp = add_frame(upage,kpage,writable);
	ASSERT(upage != NULL);
	if(temp == NULL)
		return false;
	new = malloc(sizeof * new);
	ASSERT(new != NULL);
	new->upage = upage;
	new -> idx = -1;
	he = hash_find(&(thread_current()->spt),&(new->elem));
	if(he == NULL)
		hash_insert(&thread_current()->spt,&new->elem);
	else{
		prev = hash_entry(he , struct hash_item,elem);	
		free(new);
		prev -> idx = -1;
	}
	return true;
}
bool stack_grow(void* fault_addr){
	bool writable = true;
	ASSERT(fault_addr != NULL);
	if((size_t)(PHYS_BASE-pg_round_down(fault_addr)) > (1 << 23))
		return false;
	return spt_add_page(pg_round_down(fault_addr),NULL,writable) 
		&& (pagedir_get_page(thread_current()->pagedir,fault_addr + PGSIZE) == NULL 
		&& fault_addr + PGSIZE < PHYS_BASE)? stack_grow(fault_addr + PGSIZE) : true;
}
