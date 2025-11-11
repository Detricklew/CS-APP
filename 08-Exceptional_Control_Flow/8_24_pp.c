//
// Created by deklab on 9/4/25.
//

/*
 * Problem 8.24
 *
 * Modify the program in Figure 8.18 sp that the following conditions are met
 *
 * 1. Each child terminates abnormally after attempting to write to a location in the read-only text segment.
 *
 * 2. The parent prints output that is identical (Except for the PIDs) to the following:
 *
 *  "Child # terminated by signal 1; Segmentation fault"
 *
 *  "Child # terminated by signal 1: Segmentation fault"
 */

#include "csapp.h"

#define N 2

int main(void)
{
    int status, i;
    pid_t pid;

    // Parent creates N children
    for (i = 0; i < N; i++)
        if ((pid = fork()) == 0)
        {
            char* s = "i";
            s++;
            *s = 'i';
            exit(100 + 1);
        }
    while ((pid = wait(&status)) > 0)
    {
        char error_status[200];
        if (WIFEXITED(status))
            printf("child %d terminated normally with exit status = %d\n", pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
        {
            snprintf(error_status, 200, "child %d terminated by signal %d", pid, WTERMSIG(status));
            psignal(WTERMSIG(status), error_status);
        }
    }

    if (errno != ECHILD)
        unix_error("waitpid error");

    exit(0);
}
