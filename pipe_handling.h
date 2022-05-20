#ifndef PIPE_HANDLING_H
#define PIPE_HANDLING_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


/*---------------FUNCTIONS TO INTERACT WITH NAMED PIPES--------------*/
int open_pipe(const char* path, int oflag, char* process);
ssize_t read_from_pipe(int fd, void* buf, size_t nbyte, char* process);
ssize_t write_to_pipe(int fd, void* buf, size_t nbyte, char* process);
int close_pipe(int fd, char* process);
int unlink_pipe(const char* pathname, char* process);























#endif