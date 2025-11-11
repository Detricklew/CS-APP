//
// Created by deklab on 9/4/25.
//

/*
 * Problem 8.25
 *
 * Write a version of the fgets function called tfgets, that times out after 5 seconds. The tfgets function accepts
 * the same inputs as fgets. If the user doesn't type an input line withen 5 secs tfgets returns NULL.
 * Otherwise it returns a pointer to the input line.
 */


#include "csapp.h"

sigjmp_buf buf;

pid_t parent;

void handler(int sig);
char* tfgets(char* str, int n, FILE* stream);

int main(void)
{
    parent = getppid();

    char buf[50];
    printf("Type your input and hit enter: ");
    fflush(stdout);
    if (tfgets(buf, 50, stdin) != NULL)
        printf("You inputted: %s", buf);
    else
        puts("Timed out.");

    exit(0);
}

void handler(int sig)
{
    siglongjmp(buf, 1);
}

char* tfgets(char* str, int n, FILE* stream)
{
    char* ret;

    signal(SIGALRM, handler);
    if (!sigsetjmp(buf, 1))
    {
        alarm(5);
        ret = fgets(str, n, stream);
    }
    else
    {
        ret = NULL;
    }

    return ret;
}
