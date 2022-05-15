#pragma once

#include <sys/types.h>

//#define F_DUPFD (1 << 0)         //    Duplicate file descriptor.
//#define F_DUPFD_CLOEXEC (1 << 1) //    Duplicate file descriptor with the close-on- exec flag FD_CLOEXEC set.
//#define F_GETFD (1 << 2)         //    Get file descriptor flags.
//#define F_SETFD (1 << 3)         //    Set file descriptor flags.
//#define F_GETFL (1 << 4)         //    Get file status flags and file access modes.
//#define F_SETFL (1 << 5)         //    Set file status flags.
//#define F_GETLK (1 << 6)         //    Get record locking information.
//#define F_SETLK (1 << 7)         //    Set record locking information.
//#define F_SETLKW (1 << 8)        //    Set record locking information; wait if blocked.
//#define F_GETOWN (1 << 9)        //    Get process or process group ID to receive SIGURG signals.
//#define F_SETOWN (1 << 10)       //    Set process or process group ID to receive SIGURG signals.
//#define FD_CLOEXEC (1 << 11)     //Close the file descriptor upon execution of an exec family function

#define O_RDONLY 0           //Open for reading only.
#define O_WRONLY (1 << 1)    //Open for writing only.
#define O_RDWR (1 << 2)      //Open for reading and writing.
#define O_CREAT (1 << 3)     //Create file if it does not exist.
#define O_APPEND (1 << 4)    //Set append mode.
#define O_TRUNC (1 << 5)     //Truncate flag.
#define O_NONBLOCK (1 << 6)  //Non-blocking mode.
#define O_NOFOLLOW (1 << 7)  //Do not follow symbolic links.
#define O_CLOEXEC (1 << 8)   //The FD_CLOEXEC flag associated with the new descriptor shall be set to close the file descriptor upon execution of an exec family function.
#define O_DIRECTORY (1 << 9) //Fail if file is a non-directory file.
#define O_SEARCH (1 << 10)   //Open directory for search only. The result is unspecified if this flag is applied to a non-directory file.
#define O_NOCTTY (1 << 11)   //Do not assign controlling terminal.
#define O_EXEC (1 << 12)     //Open for execute only (non-directory files). The result is unspecified if this flag is applied to a directory.
#define O_ACCMODE (1 << 13)  //Mask for file access modes.

int creat(const char *path, mode_t mode);
int fcntl(int fildes, int cmd, ...);
int open(const char *path, int oflag, ...);
int openat(int fd, const char *path, int oflag, ...);
