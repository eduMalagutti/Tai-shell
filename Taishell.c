#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <errno.h>
#include <signal.h>

#define MAX_ARGC 10
#define MAX_ARGS_SIZE 20

void exec_Child(int argc, char **argv); // Função para executar programa
int exec_BuiltinCmds(char **argv);
int exec_cd(char *arg);

int main()
{
    // Command line
    char cmdline[50];
    int argc = 0;
    char **argv = (char **)malloc(MAX_ARGC * sizeof(char *));
    for (int i = 0; i < MAX_ARGC; i++)
    {
        argv[i] = (char *)malloc(MAX_ARGS_SIZE * sizeof(char));
    }

    while (1)
    {
        // Finding current directory
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) // salva em cwb o path atual
        {
            perror("getcwd() error");
            return 1;
        }

        // colourize command line
        // \033[1;34minsert text here\033[0m
        // Reading command line
        printf("Taishell:\033[1;34m~%s\033[0m$ ", cwd);
        fgets(cmdline, 50, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';

        argc = 0;
        argv[argc] = strtok(cmdline, " ");

        while (1)
        {
            argc++;
            argv[argc] = strtok(NULL, " ");
            if (argv[argc] == NULL)
            {
                //argv = (char **)realloc(argv, (argc + 1) * sizeof(char *));
                argv[argc] = '\0';
                break;
            }
        }

        if (strcmp(argv[0], "exit") == 0)
        {
            exit(0);
        }
        else if (strcmp(argv[0], "cd") == 0)
        {
            exec_cd(argv[1]);
        }
        else
        {
            exec_Child(argc, argv);
        }

        // // Debug
        // for (int i = 0; i < argc + 1; i++)
        // {
        //     printf("argv[%d] = %s\n", i, argv[i]);
        // }
    }

    free(argv);
    return 0;
}

int exec_cd(char *arg)
{
    char lastdir[PATH_MAX]; // Initialized to zero
    char curdir[PATH_MAX];
    char path[PATH_MAX];

    if (getcwd(curdir, sizeof curdir))
    {
        // Current directory might be unreachable (not an error)
        *curdir = '\0';
    }

    if (arg == NULL)
    {
        arg = getenv("HOME");
    }
    else if (!strcmp(arg, "-"))
    {
        if (*lastdir == '\0')
        {
            fprintf(stderr, "No previous directory\n");
            return 1;
        }
        arg = lastdir;
    }
    else if (*arg == '~')
    {
        if (arg[1] == '/' || arg[1] == '\0')
        {
            snprintf(path, sizeof path, "%s%s", getenv("HOME"), arg + 1);
            arg = path;
        }
        else
        {
            // Handle ~name (expand to user's home directory)
            fprintf(stderr, "Syntax not supported: %s\n", arg);
            return 1;
        }
    }

    if (chdir(arg))
    {
        fprintf(stderr, "%s: %s: %s\n", arg, strerror(errno), path);
        return 1;
    }

    // Update last directory
    strcpy(lastdir, curdir);
    return 0;
}

void handleChildExit(int signum)
{
    // Reap the child process
    wait(NULL);
}

void exec_Child(int argc, char **argv)
{
    // Child variables
    pid_t pid;
    int status;

    signal(SIGCHLD, handleChildExit);

    pid = fork();

    setpgid(0, 0);

    if (pid == -1)
    {
        perror("Erro na criação de processo");
        exit(1);
    }

    // Condição para que o comando seja um builtin comand
    int StdPath = (argv[0][0] != '.') && (argv[0][1] != '/');

    if (pid == 0) // Child: ls, cd, echo or not bultin
    {
        if (StdPath)
        {
            exec_BuiltinCmds(argv);
        }
        else
        {
            execv(argv[0], argv);
        }
    }
    else // Parent
    {
        if (strcmp(argv[argc - 1], "&") == 0) // Run child in background
        {
            printf("%d\n", getpid());
        }
        else // Run child in foreground
        {
            wait(&status);
        }
    }
}

int exec_BuiltinCmds(char **argv)
{
    int NumberOfCmds = 5;
    char cmds[6][5] = {"ls", "ps", "echo", "jobs", "clear"};

    int ok = 0;

    if (strcmp(argv[0], "exit") == 0)
    {
        ok = 1;
        printf("Era pra ter saido\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        for (int i = 0; i < NumberOfCmds; i++)
        {
            if (strcmp(argv[0], cmds[i]) == 0)
            {
                execvp(argv[0], argv);
                ok = 1;
                return 0;
            }
            if (strcmp(argv[0], "sleep") == 0)
            {
                sleep(atoi(argv[1]));
                ok = 1;
                exit(0);
                return 0;
            }
        }
    }

    if (!ok)
    {
        fprintf(stderr, "%s: command not found\n", argv[0]);
        return -1;
    }
}