#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
void syscall_init (void);
void exit(int status);
//pid_t exec(const char *cmd_line);
//int wait(pid_t pid);
int open(const char *cmd_line);
int filesize(int fd);
int read(int fd,void *buffer,unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd,unsigned position);
unsigned tell(int fd);
void close(int fd);

#endif /* userprog/syscall.h */
