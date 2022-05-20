#include "pipe_handling.h"

//OPEN A PIPE - PRINT ERROR ON STDERR AND EXIT IF OPEN FAILS
int open_pipe(const char* path, int oflag, char* process){
    int fd;
    fd = open(path, oflag);

    if(fd == -1){
        fprintf(stderr, "%s: can't open pipe, errno:%d\n", process, errno);
        exit(EXIT_FAILURE);
    }
    
    return fd;
}

//READ FROM PIPE - PRINT ERROR ON STDERR AND EXIT IF READ FAILS
ssize_t read_from_pipe(int fd, void* buf, size_t nbyte, char* process){
    ssize_t rbytes;
    rbytes = read(fd, buf, nbyte);
    
    if((rbytes == -1) && (errno != EINTR)){
        fprintf(stderr, "%s: can't read from pipe, errno:%d\n", process, errno);
        exit(EXIT_FAILURE);
    }

    return rbytes;
}

//WRITE TO PIPE - PRINT ERROR ON STDERR AND EXIT IF WRITE FAILS
ssize_t write_to_pipe(int fd, void* buf, size_t nbyte, char* process){
    ssize_t wbytes;
    wbytes = write(fd, buf, nbyte);

    if((wbytes == -1) && (errno != EINTR)){
        fprintf(stderr, "%s: can't read from pipe, errno:%d\n", process, errno);
        exit(EXIT_FAILURE);
    }

    return wbytes;
}

//CLOSE PIPE - PRINT ERROR ON STDERR AND EXIT IF CLOSE FAILS
int close_pipe(int fd, char* process){

    int retval;
    retval = close(fd);

    if(retval == -1){
        fprintf(stderr, "%s: can't close pipe, errno:%d\n", process, errno);
        exit(EXIT_FAILURE);
    }
    
    return retval;
}

//UNLINK PIPE - PRINT ERROR ON STDERR AND EXIT IF UNLINK FAILS
int unlink_pipe(const char* pathname, char* process){

    int retval;
    retval = unlink(pathname);

    if(retval == -1){
        fprintf(stderr, "%s: can't unlink pipe, errno:%d\n", process, errno);
        exit(EXIT_FAILURE);
    }

    return retval;
}