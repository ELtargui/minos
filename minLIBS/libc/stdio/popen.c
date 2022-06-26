#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

FILE *popen(const char *command, const char *mode)
{
    if (!mode || (*mode != 'r' && *mode != 'w'))
    {
        errno = EINVAL;
        return NULL;
    }

    int pipefds[2];

    if (pipe(pipefds) < 0)
    {
        perror("pipe");
        return NULL;
    }

    pid_t child_pid = fork();
    if (child_pid < 0)
    {
        perror("fork");
        close(pipefds[0]);
        close(pipefds[1]);
        return NULL;
    }
    else if (child_pid == 0)
    {
        if (*mode == 'r')
        {
            if (dup2(pipefds[1], STDOUT_FILENO) < 0)
            {
                perror("dup2");
                exit(1);
            }
            close(pipefds[0]);
            close(pipefds[1]);
        }
        else if (*mode == 'w')
        {
            if (dup2(pipefds[0], STDIN_FILENO) < 0)
            {
                perror("dup2");
                exit(1);
            }
            close(pipefds[0]);
            close(pipefds[1]);
        }

        if (execl("/bin/sh", "sh", "-c", command, NULL) < 0)
            perror("execl");
        exit(1);
    }

    FILE *file = NULL;
    if (*mode == 'r')
    {
        file = fdopen(pipefds[0], mode);
        close(pipefds[1]);
    }
    else if (*mode == 'w')
    {
        file = fdopen(pipefds[1], mode);
        close(pipefds[0]);
    }

    file->popen_pid = child_pid;

    return file;
}

int pclose(FILE *stream)
{
    int status;
    if (waitpid(stream->popen_pid, &status, 0) < 0)
        status = -1;

    fclose(stream);   
    return status;
}
