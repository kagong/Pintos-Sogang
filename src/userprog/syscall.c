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

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
//	  printf ("system call!!\n");
	int *arg=(int *)(f->esp);
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
			if(arg[1] != NULL && is_user_vaddr(arg[1]) && pagedir_get_page(thread_current()->pagedir,arg[1]) != NULL)
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
			if(arg[1] != NULL && is_user_vaddr(arg[1]) && pagedir_get_page(thread_current()->pagedir,arg[1]) != NULL)
				f->eax = open(arg[1]);
			else 
				exit(-1);
			break;
		case SYS_FILESIZE:
			f->eax = filesize(arg[1]);
			break;
		case SYS_READ:
			if(arg[2] != NULL && is_user_vaddr(arg[2]) && pagedir_get_page(thread_current()->pagedir,arg[2]) != NULL)
				f->eax = read(arg[1],(void *)arg[2],(unsigned)arg[3]);
			else 
				exit(-1);
			break;
		case SYS_WRITE:
			if(arg[2] != NULL && is_user_vaddr(arg[2]) && pagedir_get_page(thread_current()->pagedir,arg[2]) != NULL)
				f->eax = write(arg[1],(void *)arg[2],(unsigned)arg[3]);
			else 
				exit(-1);
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
	if (f == NULL)
		return -1;
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
	if(fd == 0)
	{
		uint8_t *ptr = (uint8_t*)buffer;
		int i;
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
		return file_read(f,buffer,size);
	}
}
int write(int fd, const void *buffer, unsigned size)
{
	if(fd == 1)
	{
		putbuf(buffer,size);
		return size;
	}
	else if(fd == 0)
		return 0;
	else
	{
		struct file *f = file_find_fd(fd);
		if(f == NULL)
			return 0;
		if(find_thread_name(file_get_name(f)))
		{
			file_deny_write(f);
			int retval = file_write(f,buffer,size);
			file_allow_write(f);
			return retval;
		}

		return file_write(f,buffer,size);
		
	}

}
void seek(int fd,unsigned position)
{
	if(fd <= 1)
		return ;
	struct file *f = file_find_fd(fd);
	if(f == NULL)
		return ;
	return file_seek(f,position);
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
