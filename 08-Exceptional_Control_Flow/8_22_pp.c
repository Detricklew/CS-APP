//
// Created by deklab on 9/4/25.
//

/*
 * Practice Problem 8.22
 *
 * Write your own version of the Unix system function
 */

#include "csapp.h"

int mysystem(char* command);


int mysystem(char* command)
{
    char* argv2[4] = {
        "/bin/sh",
        "-c",
        command,
        NULL,
    };

    sigset_t mask, prevmask;

    sigfillset(&mask);

    sigprocmask(SIG_BLOCK, &mask, &prevmask);

    if (fork() == 0)
    {
        if (execve("/bin/sh", argv2, (char*[]){NULL}) == -1)
        {
            exit(-1);
        };
        exit(0);
    }

    sigprocmask(SIG_UNBLOCK, &mask, &prevmask);

    int status;

    wait(&status);


    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return -1;
}

int main(int argc, char** argv, char** envp)
{
    if (argc <= 1)
    {
        puts("no args");
        return -1;
    }
    printf("command exited with %d status \n", mysystem(argv[1]));
    return 0;
}
