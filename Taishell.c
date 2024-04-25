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
#define MAX_ARGV_SIZE 20

//Declaração das funções
void exec_Child(int argc, char **argv, int isComand);
int exec_cd(char *argv);
void exec_comand(int argc, char **argv);

int main()
{
    // Command line
    char cmdline[50];
    int argc = 0;
    char **argv = (char **)malloc(MAX_ARGC * sizeof(char *));
    for (int i = 0; i < MAX_ARGC; i++)
    {
        argv[i] = (char *)malloc(MAX_ARGV_SIZE * sizeof(char));
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
        // \033[1;35mTaishell:\033[0m
        // Reading command line
        printf("\033[1;35mTaishell:\033[0m\033[1;34m~%s\033[0m$ ", cwd);
        fgets(cmdline, 50, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';

        //Argc = Contador de argumentos
        argc = 0;
        argv[argc] = strtok(cmdline, " ");

        while (1)
        {
            argc++;
            argv[argc] = strtok(NULL, " "); // argv vai ser um vetor de strings guardando todos os argumentos
            if (argv[argc] == NULL)
            {
                // argv = (char **)realloc(argv, (argc + 1) * sizeof(char *));
                break;
            }
        }

        // Condição para que o comando seja um builtin comand
        int isCommand = (argv[0][0] != '.') && (argv[0][1] != '/');
        if (isCommand)
        {
            exec_comand(argc, argv);
        }
        else
        {
            exec_Child(argc, argv, isCommand);
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

void exec_Child(int argc, char **argv, int isComand)
{
    // Child variables
    pid_t pid;
    int status;

    // signal(SIGCHLD, handleChildExit);

    pid = fork();

    if (pid == -1)
    {
        perror("Erro na criação de processo");
        exit(1);
    }

    if (pid == 0) // Child: ls, cd, echo or not bultin
    {
        if (isComand)
        {
            execvp(argv[0], argv);
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
        }
        else // Run child in foreground
        {
            wait(&status);
        }
    }
}

int exec_cd(char *argv)
{
    char lastdir[PATH_MAX]; // Initialized to zero
    char curdir[PATH_MAX];
    char path[PATH_MAX];

    if (getcwd(curdir, sizeof curdir))
    {
        // Current directory might be unreachable (not an error)
        *curdir = '\0';
    }

    if (argv == NULL)
    {
        argv = getenv("HOME");
    }
    else if (!strcmp(argv, "-"))
    {
        if (*lastdir == '\0')
        {
            fprintf(stderr, "No previous directory\n");
            return 1;
        }
        argv = lastdir;
    }
    else if (*argv == '~')
    {
        if (argv[1] == '/' || argv[1] == '\0')
        {
            snprintf(path, sizeof path, "%s%s", getenv("HOME"), argv + 1);
            argv = path;
        }
        else
        {
            // Handle ~name (expand to user's home directory)
            fprintf(stderr, "Syntax not supported: %s\n", argv);
            return 1;
        }
    }

    if (chdir(argv))
    {
        fprintf(stderr, "%s: %s: %s\n", argv, strerror(errno), path);
        return 1;
    }

    // Update last directory
    strcpy(lastdir, curdir);
    return 0;
}

void exec_comand(int argc, char **argv)
{
    int isCommand = 1;

    if (strcmp(argv[0], "exit") == 0) // Se o comando for exit
    {
        exit(0);
    }
    else if (strcmp(argv[0], "cd") == 0) // Se o comando for cd
    {
        exec_cd(argv[1]);
    }
    else
    {
        exec_Child(argc,argv,isCommand);
    }
}
