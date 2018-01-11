#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include <stdio.h>
#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include "threads/malloc.h"
#ifdef USERPROG
#include "userprog/pagedir.h"
#endif
#define PTE_P 0x1               /* 1=present, 0=not present. */
struct list frame_list;
void frame_init(){
	list_init(&frame_list);
}
struct frame* add_frame(void *upage, void* kpage,bool writable){
	struct frame *new = NULL;
	ASSERT(upage != NULL);
	kpage = kpage == NULL ? palloc_get_page(PAL_USER) : kpage;
	if(kpage  == NULL)
		kpage = evit_frame(PAL_USER,upage);
	ASSERT(kpage != NULL);

	if(!pagedir_set_page(thread_current()->pagedir,upage,kpage,writable))
	{
		palloc_free_page(kpage);
		return NULL;
	}
	new = malloc(sizeof * new);
	new -> upage = upage;
	new -> paddr = kpage;
	new -> t = thread_current();
	list_push_back(&frame_list,&new->elem);
	return new;
}
void *evit_frame(enum palloc_flags flag,void *upage){
	struct frame *victim = NULL;
	struct list_elem *temp = NULL;
	struct frame* cur=NULL;
	int idx = 0;
	if(list_empty(&frame_list))
		return NULL;
	for(temp = list_front(&frame_list);;temp = list_next(temp)){
		cur = list_entry(temp,struct frame,elem);
		if(pagedir_is_accessed(cur->t->pagedir,cur->upage)){
			pagedir_set_accessed(cur->t->pagedir,cur->upage,false);
			if(list_next(temp) == list_end(&frame_list)){
				victim = list_entry(list_pop_front(&frame_list),struct frame,elem);
				break;
			}
		}
		else{
			victim = cur;			
			list_remove(&cur->elem);
			break;
		}
	}

	if(victim -> t != thread_current())
	{
		pagedir_set_page(thread_current()->pagedir,upage,victim->paddr,true);
		idx = swap_out(upage);
		pagedir_clear_page(thread_current()->pagedir,upage);
	}
	else
		idx = swap_out(victim->upage);
	list_remove(&victim->elem);
	palloc_free_page(victim->paddr);
	pagedir_clear_page(victim->t->pagedir,victim->upage);	
	spt_clear_frame_set_idx(victim,idx);
	free(victim);
	return palloc_get_page(flag);
}
void free_frame(struct thread* t){
	struct list_elem *temp=NULL,*next=NULL;
	struct frame* cur=NULL;
	if(list_empty(&frame_list))
		return ;
	for(temp = list_front(&frame_list);temp != list_end(&frame_list);){

		cur = list_entry(temp,struct frame,elem);
	   	if(cur -> t == t){
			next = list_next(temp);
			list_remove(temp);
			free(cur);
			temp = NULL;
		}
		if(temp == NULL)
			temp = next;
		else
			temp = list_next(temp);
	}
}
