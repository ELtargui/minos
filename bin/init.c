#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char const *argv[])
{
    open("/dev/dbg", O_RDONLY);
    open("/dev/dbg", O_WRONLY);
    open("/dev/dbg", O_WRONLY);

    printf("hello world!\n");

    int child = fork();

    if (!child)
    {   
        const char *args[] = 
        {
            "/bin/wm",
            "hello",
            NULL,
        };

        execve(args[0], args, NULL);
        return 1234;
    }

    printf("waiting for childs...\n");
    int st = 0;
    while (1)
    {
        int c = waitpid(-1, &st, 0);
        if (c == -1)
        {
            perror("waitpid");
            break;
        }

        printf("child exited [%d::%d]\n", c, st);
    }
    return 0;
}
