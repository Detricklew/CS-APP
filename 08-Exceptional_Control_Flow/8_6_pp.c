//
// Created by deklab on 9/5/25.
//

/*
 * Practice Problem 8.6
 *
 * Write a program called "myecho" that prints out its command-line arguments and environment variables
 *
 */

#include "csapp.h"

int main(int argc, char** argv, char** envp)
{
    int arg_size = argc;

    puts("Command-Line arguments: ");

    for (int i = 0; i < arg_size; i++)
    {
        printf("argv[%2d]: %s\n", i, argv[i]);
    }

    puts("\nEnviroment variables");

    for (int i = 0; envp[i] != NULL; i++)
    {
        printf("envp[%2d]: %s\n", i, envp[i]);
    }

    return 0;
}
