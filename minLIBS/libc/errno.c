#include <stdio.h>
#include <errno.h>
#include <string.h>

int errno = 0;

static char *sys_errlist[] = {
    "INAVALIDE ERR",
    "Argument list too long",
    "Permission denied",
    "Address in use",
    "Address not available",
    "Address family not supported",
    "Resource unavailable, try again",
    "Connection already in progress",
    "Bad file descriptor",
    "Bad message",
    "Device or resource busy",
    "Operation canceled",
    "No child processes",
    "Connection aborted",
    "Connection refused",
    "Connection reset",
    "Resource deadlock would occur",
    "Destination address required",
    "Mathematics argument out of domain of function",
    "Reserved",
    "File exists",
    "Bad address",
    "File too large",
    "Host is unreachable",
    "Identifier removed",
    "Illegal byte sequence",
    "Operation in progress",
    "Interrupted function",
    "Invalid argument",
    "I/O error",
    "Socket is connected",
    "Is a directory",
    "Too many levels of symbolic links",
    "File descriptor value too large",
    "Too many links",
    "Message too large",
    "Reserved",
    "Filename too long",
    "Network is down",
    "Connection aborted by network",
    "Network unreachable",
    "Too many files open in system",
    "No buffer space available",
    "No message is available on the STREAM head read queue",
    "No such device",
    "No such file or directory",
    "Executable file format error",
    "No locks available",
    "Reserved",
    "Not enough space",
    "No message of the desired type",
    "Protocol not available",
    "No space left on device",
    "No STREAM resources",
    "Not a STREAM",
    "Functionality not supported",
    "The socket is not connected",
    "Not a directory or a symbolic link to a directory",
    "Directory not empty",
    "State not recoverable",
    "Not a socket",
    "Not supported",
    "Inappropriate I/O control operation",
    "No such device or address",
    "Value too large to be stored in data type",
    "Previous owner died",
    "Operation not permitted",
    "Broken pipe",
    "Protocol error",
    "Protocol not supported",
    "Protocol wrong type for socket",
    "Result too large",
    "Read-only file system",
    "Invalid seek",
    "No such process",
    "Reserved",
    "Stream ioctl timeout",
    "Connection timed out",
    "Text file busy",
    "try again",
    "Cross-device link",
};

void perror(const char *s)
{
    if (s && *s != 0)
    {
        fprintf(stderr, "%s: ", s);
    }

    fprintf(stderr, "%s\n", strerror(errno));
}

char *strerror(int errnum)
{
    if (errnum < 0 || errnum > (int)(sizeof(sys_errlist) / sizeof(char *)))
        return NULL;

    return sys_errlist[errnum];
}
