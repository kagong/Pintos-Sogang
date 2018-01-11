#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <console.h>
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include <syscall-nr.h>
#include <user/syscall.h>
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/off_t.h"
#include "vm/page.h"
#include "vm/swap.h"
static void syscall_handler (struct intr_frame *);
struct lock read_write_lock;
void
syscall_init (void) 
{
	lock_init(&read_write_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
void* esp;
static void
syscall_handler (struct intr_frame *f) 
{
	int *arg=(int *)(f->esp),i;
	uint32_t *pd = thread_current()->pagedir;
	for(i=0;i<4;arg++ && i++)
		if(arg == NULL || !is_user_vaddr(arg) || pagedir_get_page(pd,arg) == NULL)
			exit(-1);
	arg = (int*)(f->esp);
	switch(arg[0])
	{
		case SYS_HALT:
			shutdown_power_off();
			break;
		case SYS_EXIT:
			if(is_user_vaddr(arg+1))
				exit(arg[1]);
			else
				exit(-1);
			break;
		case SYS_EXEC:
			if(arg[1] != NULL && is_user_vaddr(arg[1]))
				f->eax=exec((char *)arg[1]);
			else
			{
				f->eax = -1;
				exit(-1);
			}
			break;
		case SYS_WAIT:
			f->eax=wait((pid_t)arg[1]);
			break;
		case SYS_CREATE:
			if(arg[1] != NULL)
				f->eax = filesys_create(arg[1],arg[2]);
			else 
				exit(-1);
			break;
		case SYS_REMOVE:
			f->eax = filesys_remove(arg[1]);
			break;
		case SYS_OPEN:
			if(arg[1] != NULL && is_user_vaddr(arg[1]))
				f->eax = open(arg[1]);
			
			else 
				exit(-1);
			break;
		case SYS_FILESIZE:
			f->eax = filesize(arg[1]);
			break;
		case SYS_READ:
				esp = f->esp;
				f->eax = read(arg[1],(void *)(arg[2]),(unsigned)(arg[3]));
			break;
		case SYS_WRITE:
				f->eax = write(arg[1],(void *)arg[2],(unsigned)arg[3]);
			break;
		case SYS_SEEK:
			seek(arg[1],arg[2]);
			break;
		case SYS_TELL:
			f->eax = tell(arg[1]);
			break;
		case SYS_CLOSE:
			close(arg[1]);
			break;
		case SYS_PIBONACCI:
			f->eax = pibonacci(arg[1]);
			break;
		case SYS_SUM_OF_FOUR_INTEGERS:
			f -> eax = sum_of_four_integer(arg[1],arg[2],arg[3],arg[4]);
			break;
			//----------------------------
		case SYS_MMAP:
			f->eax = mmap(arg[1],arg[2]);
			break;
			//----------------------------
		default:
			printf("not yet system call!\n\n");
			break;
		
	}
}
void exit(int status)
{
	char *ptr;
	printf("%s: exit(%d)\n",strtok_r(thread_name()," ",&ptr),status );
	pinfo_set_status(status);
	thread_exit();
}
pid_t exec(const char *cmd_line)
{
	struct thread *t = NULL;
	tid_t tid = process_execute(cmd_line);
	t=find_thread(tid);
	if(t == NULL)
		return -1;
	sema_down(&t->sema);
	if(pinfo_get_status(tid) == -1&& find_thread(tid) == NULL)
		return -1;
	return (pid_t)tid;

}
int wait(pid_t pid)
{
	return process_wait((tid_t)pid);
}
int open(const char *cmd_line)
{
	struct file* f =filesys_open(cmd_line);
	if (f == NULL){
		return -1;
	}
	return file_get_fd(f);

}
int filesize(int fd)
{
	if(fd <= 1)
		return 0;
	struct file *f = file_find_fd(fd);
	if(f==NULL)
		return 0;
	return file_length(f);
}
int read(int fd,void *buffer,unsigned size)
{
	int retval;
	size_t num = (size_t)size;
	void *local = buffer;
	while(local != NULL){
		if(local == NULL || !is_user_vaddr(local))
			exit(-1);
		else if(pagedir_get_page(thread_current()->pagedir,local) == NULL){
			if(is_in_spt(local)){
				spt_swap(local);
			}
			else if(local>(esp - 32)){
				if(!stack_grow(local))
					exit(-1);	
			}
			else
				exit(-1);
		}
		if(num == 0)
			local = NULL;
		else if(num > PGSIZE){
			num -= PGSIZE;
			local += PGSIZE;
		}
		else
		{
			local = buffer + size -1;
			num = 0;
		}
	}
	if(fd == 0)
	{
		uint8_t *ptr = (uint8_t*)buffer;
		uint8_t i;
		for(i=0;i<size;++i)
			*(ptr++) = input_getc();
		return size;
	}
	else if(fd == 1)
		return 0;
	else
	{
		struct file *f = file_find_fd(fd);
		if(f == NULL)
			return 0;
		lock_acquire(&read_write_lock);
		retval = file_read(f,buffer,size);
		lock_release(&read_write_lock);
		return retval;
	}
}
int write(int fd, const void *buffer, unsigned size)
{
	size_t num = (size_t)size;
	void *local = buffer;
	while(local != NULL){
		if(local == NULL || !is_user_vaddr(local))
			exit(-1);
		else if(pagedir_get_page(thread_current()->pagedir,local) == NULL){
			if(is_in_spt(local)){
				spt_swap(local);
			}
			else if(local>(esp - 32)){
				if(!stack_grow(local))
					exit(-1);	
			}
			else
				exit(-1);
		}
		if(num == 0)
			local = NULL;
		else if(num > PGSIZE){
			num -= PGSIZE;
			local += PGSIZE;
		}
		else
		{
			local = buffer + size -1;
			num = 0;
		}
	}
	if(fd == 1)
	{
		putbuf(buffer,size);
		return size;
	}
	else if(fd == 0)
		return 0;
	else
	{
		int retval;
		struct file *f = file_find_fd(fd);
		if(f == NULL)
			return 0;
		lock_acquire(&read_write_lock);
		if(find_thread_name(file_get_name(f)))
		{
			file_deny_write(f);
			retval = file_write(f,buffer,size);
			file_allow_write(f);
		}
		else
			retval = file_write(f,buffer,size);
		lock_release(&read_write_lock);
		return retval;
		
	}

}
void seek(int fd,unsigned position)
{
	if(fd <= 1)
		return ;
	struct file *f = file_find_fd(fd);
	if(f == NULL)
		return ;
	file_seek(f,position);
}
unsigned tell(int fd)
{
	if(fd <= 1)
		return 0;
	struct file *f = file_find_fd(fd);
	if(f == NULL)
		return 0;
	return file_tell(f);
}
void close(int fd)
{
	struct file *f = file_find_fd(fd);
	if(f == NULL)
		exit(-1);
	else
		file_close(f);
}
//-----------------------------------------------
int mmap (int fd, void *addr){
	int size = filesize(fd);
	thread_current() -> mmap = true;
	thread_current() -> mfd = fd;
	thread_current() -> maddr = addr;
	thread_current() -> msize = size;

	while(size > 0){
		if(size < PGSIZE){
			spt_add_page(addr,NULL,1);
			read(fd,addr,size);
			return 1;
		}
		spt_add_page(addr,NULL,1);
		read(fd,addr,PGSIZE);
		addr += PGSIZE;
		size -= PGSIZE;
	}
	return 1;
}
//--------------------------------------------
int pibonacci(int n)
{
	return n < 3 ? 1 : pibonacci(n - 1) + pibonacci( n - 2);
}
int sum_of_four_integer(int a,int b,int c,int d)
{
	return a+b+c+d;
}
