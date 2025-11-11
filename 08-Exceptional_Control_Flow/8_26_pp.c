//
// Created by deklab on 9/4/25.
//

/*
 * Problem 8.26
 */

#include "csapp.h"
#define MAXARGS 128
#define MAXJOBS 128
#define MAXERRLEN 512

typedef enum
{
    RUNNING = 0,
    SUSPENDED,
    TERMINATED,
    RESTARTED,
    ERROR
} status_t;

typedef struct
{
    pid_t pid;
    status_t status;
    char cmdline[MAXLINE];
} Job_t;

typedef struct Node
{
    Job_t job;
    int job_id;
    struct Node* next;
} Node_t;

typedef struct
{
    Node_t* front;
    Node_t* back;
    int size;
    int _count;
} Queue_t;

jmp_buf buffer;

pid_t main_pid;

int newline = 0;

Queue_t handler = {NULL, NULL, 0, 1};
/*
 * Signal handlers and functions
 */
void sigchild_handler(int sig);
void sigint_handler(int sig);
void reap_children(void);
void kill_children(Queue_t* jobs, int signal);

/*
 * job queue functions
 */

int add_Job(Queue_t* jobs, int pid, char* cmdline);
int remove_job_by_jid(Queue_t* jobs, int jid);
int remove_job_by_pid(Queue_t* jobs, int pid);
Job_t* find_job_by_pid(Queue_t* jobs, int pid);
Job_t* find_job_by_jid(Queue_t* jobs, int jid);
void signal_jobs(Queue_t* jobs, int signal);
void print_jobs(Queue_t* jobs);

/*
 * Job/Node helper functions
 */
Job_t job_create(int pid, char* argv, status_t status);
Node_t* Create_Node(Job_t job, int job_id, Node_t* next);
void job_update_status(Job_t* job, status_t status);
void job_free(Job_t* job);
void print_job(Node_t* job);
void free_node(Node_t* node);

/*
 * Shell helper functions
 */
void eval(char* cmdline);
int parseline(char* buf, char** argv, char* argbuf);
int builtin_command(char** argv);

/*
 * char array functions
 */
void arg_copy(char** dest, char** source, int size);
void free_args(char** args);

/*
 * Shell built-in functions
 */
void foo(char** argv);
void bg(char** argv);
void fg(char** argv);

int main(void)
{
    char cmdline[MAXLINE];
    main_pid = getpid();


    switch (setjmp(buffer))
    {
    case 1:
        puts("");
    default:
        newline = 1;
        break;
    }

    Signal(SIGCHLD, sigchild_handler);
    Signal(SIGINT, sigint_handler);

    while (1)
    {
        if (newline)
        {
            newline = 0;
            fputs("DekShell:",stdout);
        }
        fflush(stdout);
        fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
        {
            exit(0);
        }
        eval(cmdline);
    }
}

void eval(char* cmdline)
{
    char* argv[MAXARGS]; // initialize argument list array
    char buf[MAXLINE]; // hold modified command line buffer
    char cmdbuf[MAXLINE]; // modified string of argument array
    int bg; // bool for whether a job should run in background or not
    pid_t pid; // process id structure

    strcpy(buf, cmdline);

    memset(cmdbuf, 0, sizeof(cmdbuf)); // cmdline persists after first call
    bg = parseline(buf, argv, cmdbuf);

    if (*argv == NULL)
        return;
    sigset_t cur;
    sigaddset(&cur, SIGCHLD);

    sigaddset(&cur, SIGINT);
    sigprocmask(SIG_BLOCK, &cur, NULL);
    if ((pid = Fork()) == 0)
    {
        if (!builtin_command(argv))
        {
            if (execve(argv[0], argv, environ) < 0)
            {
                printf("\n%s :command not found. \n", argv[0]);
                exit(1);
            }
        }
        exit(0);
    }

    add_Job(&handler, pid, cmdbuf);
    sigprocmask(SIG_UNBLOCK, &cur, NULL);

    if (!bg)
    {
        while (find_job_by_pid(&handler, pid) != NULL);
    }
    else
    {
        print_jobs(&handler);
    }

    newline = 1;
}

int parseline(char* buf, char** argv, char* argbuf)
{
    char* delimiter;
    int argc = 0;
    int bg;
    int newbuf = 0;

    if (argbuf != NULL)
    {
        newbuf = 1;
    }

    if (*buf == '\n')
    {
        *argv = NULL;
        return 0;
    }

    buf[strlen(buf) - 1] = ' ';

    // skip leading spaces
    while (*buf && (*buf == ' '))
        buf++;

    while ((delimiter = strchr(buf, ' ')))
    {
        argv[argc++] = buf; // point array to beginning of string
        *delimiter = '\0'; // null terminated current string
        if (newbuf)
        {
            if (argc != 1) strcat(argbuf, " ");
            strcat(argbuf, buf);
        }
        buf = delimiter + 1; // move pointer to the next space
        while (*buf && (*buf == ' ')) // skip leading spaces
            buf++;
    }

    argv[argc] = NULL; //set last arg to null

    if ((bg = (*argv[argc - 1] == '&')) != 0)
    {
        argv[--argc] = NULL;
    }

    return bg;
}

int builtin_command(char** argv)
{
    if (!strcmp(*argv, "quit"))
    {
        kill(main_pid,SIGTERM);
        return 1;
    }
    if (!strcmp(*argv, "foo"))
    {
        foo(argv);
        return 1;
    }
    if (!strcmp(*argv, "bg"))
    {
        bg(argv);
        return 1;
    }
    if (!strcmp(*argv, "fg"))
    {
        fg(argv);
        return 1;
    }
    if (!strcmp(*argv, "jobs"))
    {
        print_jobs(&handler);
        return 1;
    }
    if (!strcmp(*argv, "&")) // ignores single &
        return 1;
    return 0;
}

void reap_children(void)
{
    int status;
    char error_status[MAXERRLEN];
    pid_t pid;

    while ((pid = Waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        if (WIFSIGNALED(status) | WIFEXITED(status))
        {
            remove_job_by_pid(&handler, pid);
            fflush(stdout);

            if (WIFSIGNALED(status))
            {
                newline = 1;
                snprintf(error_status, MAXERRLEN, "/n Job %d terminated by signal: %d", pid, WTERMSIG(status));
                psignal(WTERMSIG(status), error_status);
            }

            if (WIFEXITED(status))
            {
                if (WEXITSTATUS(status) != 0) newline = 1;
            }
        }

        if (WIFCONTINUED(status))
        {
            job_update_status(find_job_by_pid(&handler, pid), RESTARTED);
        }

        if (WIFSTOPPED(status))
        {
            job_update_status(find_job_by_pid(&handler, pid), SUSPENDED);
        }
    }

    if (errno != ECHILD && pid < 0 && !WIFSTOPPED(status) && !WIFCONTINUED(status))
    {
        printf("%d ??? \n", pid);
        unix_error("waitpid error");
    }
}

int remove_job_by_pid(Queue_t* jobs, int pid)
{
    if (jobs->front == NULL || jobs->size == 0)
    {
        puts("Job_handler error: deletion: Job size 0");
        return -1;
    }

    Node_t* prev = NULL;
    Node_t* cur = jobs->front;
    while (cur != NULL)
    {
        if (cur->job.pid == pid)
        {
            if (prev == NULL)
            {
                free_node(cur);
                cur = NULL;
                jobs->back = cur;
                jobs->front = cur;
            }
            else
            {
                prev->next = cur->next;
                if (cur == jobs->back)
                    jobs->back = prev;
                free_node(cur);
            }
            return --jobs->size;
        }
        prev = cur;
        cur = cur->next;
    }
    puts("Job_handler error: deletion: Job not found.");
    return -1;
}

int add_Job(Queue_t* jobs, int pid, char* cmdline)
{
    if (jobs->size == MAXJOBS)
    {
        puts("Job_handler error: addition: Max jobs added.");
        return -1;
    }

    if (jobs->front == NULL)
    {
        jobs->front = Create_Node(job_create(pid, cmdline, RUNNING), jobs->_count, jobs->front);
        jobs->back = jobs->front;
    }
    else
    {
        jobs->back->next = Create_Node(job_create(pid, cmdline, RUNNING), jobs->_count, NULL);
        jobs->back = jobs->back->next;
    }

    jobs->size++;
    return jobs->_count++;
}

Job_t job_create(int pid, char* argv, status_t status)
{
    Job_t ret;

    ret.pid = pid;
    strcpy(ret.cmdline, argv);
    ret.status = status;

    return ret;
}

void arg_copy(char** dest, char** source, int size)
{
    int i = 0;
    while (*source != NULL && i < size)
    {
        *dest = malloc(sizeof(char) * strlen(*source));
        strcpy(*dest, *source);
        dest++;
        source++;
        i++;
    }

    if (i == size)
    {
        dest = NULL;
        return;
    }

    *dest = NULL;
}

void free_args(char** args)
{
    while (*args != NULL)
    {
        free(*args);
    }
}

void job_free(Job_t* job)
{
    //free_args(job->cmdline);
}

Node_t* Create_Node(Job_t job, int job_id, Node_t* next)
{
    Node_t* ret = malloc(sizeof(Node_t));

    if (ret == NULL)
    {
        return ret;
    }
    ret->job_id = job_id;
    ret->job = job;
    ret->next = next;

    return ret;
}

void free_node(Node_t* node)
{
    job_free(&node->job);
    free(node);
}

void print_jobs(Queue_t* jobs)
{
    Node_t* cur = jobs->front;
    while (cur != NULL)
    {
        print_job(cur);
        cur = cur->next;
    }
}

void print_job(Node_t* job)
{
    // typedef struct
    // {
    //     int pid;
    //     char* ARGS[MAXARGS];
    // } Job_t;
    char stat[40];


    if (job->job.status == RUNNING)
    {
        strcpy(stat, "Running");
    }
    else if (job->job.status == SUSPENDED)
    {
        strcpy(stat, "Suspended");
    }
    else if (job->job.status == RESTARTED)
    {
        strcpy(stat, "RESTARTED");
    }
    else if (job->job.status == TERMINATED)
    {
        strcpy(stat, "Terminated");
    }
    else
    {
        strcpy(stat, "Error");
    }
    printf("[%d] " "%d  " "%s  " "%s\n", job->job_id, job->job.pid, stat, job->job.cmdline);
}

void sigchild_handler(int sig)
{
    reap_children();
}

void foo(char** argv)
{
    char* secs = argv[1];
    char* endptr;
    long int numsec;

    if (secs == NULL)
        return;

    numsec = strtol(secs, &endptr, 10);

    if (*endptr == '\0')
        sleep(numsec);
}

void sigint_handler(int sig)
{
    kill_children(&handler, SIGSTOP);
    siglongjmp(buffer, 1);
}

void kill_children(Queue_t* jobs, int signal)
{
    Node_t* cur = jobs->front;
    while (cur != NULL)
    {
        kill(cur->job.pid, signal);
        cur = cur->next;
    }
}

void job_update_status(Job_t* job, status_t status)
{
    if (job == NULL) return;

    job->status = status;
}

Job_t* find_job_by_pid(Queue_t* jobs, int pid)
{
    if (jobs->front == NULL || jobs->size == 0)
    {
        return NULL;
    }
    Node_t* cur = jobs->front;
    while (cur != NULL)
    {
        if (cur->job.pid == pid)
        {
            return &cur->job;
        }
        cur = cur->next;
    }
    return NULL;
}

Job_t* find_job_by_jid(Queue_t* jobs, int jid)
{
    if (jobs->front == NULL)
    {
        puts("Job_handler error: find by pid: Job out of range.");
        return NULL;
    }

    Node_t* cur = jobs->front;

    while (cur != NULL)
    {
        if (cur->job_id == jid)
        {
            return &cur->job;
        }
        cur = cur->next;
    }

    puts("Job_handler error: find by jid: Job not found.");
    return NULL;
}

void bg(char** argv)
{
    if (argv[1] == NULL)
    {
        puts("Not enough arguments");
        return;
    }


    int is_job = 0;
    char* secs;

    if (argv[1][0] == '%')
    {
        is_job = 1;
        char** cpy = argv;
        secs = ++cpy[1];
    }
    else
    {
        secs = argv[1];
    }

    char* endptr;
    int id;

    if (secs == NULL)
        return;

    id = strtol(secs, &endptr, 10);

    if (*endptr == '\0')
    {
        Job_t* job;
        if (is_job)
        {
            job = find_job_by_jid(&handler, id);
        }
        else
        {
            job = find_job_by_pid(&handler, id);
        }

        if (job == NULL)
        {
            printf("Error in finding job");
            exit(1);
        }

        if (job->status == RUNNING)
        {
            return;
        }

        if (job->status == SUSPENDED)
        {
            kill(job->pid, SIGCONT);
            //TODO check if process is actually running
        }


        if (job->status == TERMINATED)
        {
            //block signals here
            remove_job_by_pid(&handler, job->pid);
            eval(job->cmdline);
        }
    }
}

void fg(char** argv)
{
    if (argv[1] == NULL)
    {
        puts("Not enough arguments");
        return;
    }


    int is_job = 0;
    char* secs;

    if (argv[1][0] == '%')
    {
        is_job = 1;
        char** cpy = argv;
        secs = ++cpy[1];
    }
    else
    {
        secs = argv[1];
    }

    char* endptr;
    int id;

    if (secs == NULL)
        return;

    id = strtol(secs, &endptr, 10);

    if (*endptr == '\0')
    {
        pid_t pid;
        Job_t* job;
        if (is_job)
        {
            job = find_job_by_jid(&handler, id);
        }
        else
        {
            job = find_job_by_pid(&handler, id);
        }

        if (job == NULL)
        {
            printf("Error in finding job");
            exit(1);
        }

        int status;
        pid = job->pid;
        if (job->status == RUNNING || job->status == RESTARTED)
        {
            while (find_job_by_pid(&handler, pid) != NULL);
        }

        if (job->status == SUSPENDED)
        {
            kill(job->pid, SIGCONT);
            while (find_job_by_pid(&handler, pid) != NULL);
        }


        // if (job->status == TERMINATED)
        // {
        //     //block signals here
        //     char cmdline[MAXLINE];
        //     strcpy(cmdline, job->cmdline);
        //     remove_job_by_pid(&handler, job->pid);
        //     eval(cmdline);
        // }
    }
}
