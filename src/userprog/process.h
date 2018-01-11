#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

int pinfo_get_status(tid_t target);
void pinfo_set_status(int status);
void new_process_info(tid_t);
void free_pinfo(void);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
