//
// Created by deklab on 9/4/25.
//

/*
 * Practice Problem 8.20
 *
 * Use "execve" to write a program called myls whose behavior is identical to the "/bin/ls" program. Your program
 * should accept the same command line arguments, interpret the identical environment variables, and produce the
 * identical output.
 *
 */

#include "csapp.h"


int main(int argc, char** argv, char** envp)
{
    if (execve("/bin/ls", argv, envp) == -1)
    {
        puts("invalid command");
        exit(0);
    }
}
