#ifndef VM_FRAME_H
#define VM_FRAME_H
#include<list.h>
#include "threads/thread.h"
#include "threads/palloc.h"
struct frame{
	void *upage;
	void *paddr;
	struct thread * t;
	struct list_elem elem;
};
void frame_init(void);
struct frame* add_frame(void *upage, void* kpage,bool writable);
void* evit_frame(enum palloc_flags,void *upage);
void free_frame(struct thread* t);
#endif
