#ifndef VM_SWAP_H
#define VM_SWAP_H
#include <stddef.h>
void swap_init(void);
void swap_in(void *upage,size_t idx);
int swap_out(void *upage);
#endif
