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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_COMMAND_LINE 50
#define MAX_ARGV_SIZE 50
#define MAX_ARG_SIZE 50

// Declaração das funções
void exec_Child(int argc, char **argv, int redirectionIndex);
int exec_cd(char *argv);
void handler(int signal);

int main()
{
    while (1)
    {
        // Variavel para redirecionamento de output
        int redirectionIndex = -1;

        // Strings para a linha de comando
        char cmdline[MAX_COMMAND_LINE];

        char *argv[MAX_ARGV_SIZE];
        for (int i = 0; i < MAX_ARGV_SIZE; i++)
        {
            argv[i] = (char *)malloc(MAX_ARG_SIZE * sizeof(char));
            if (argv[i] == NULL)
            {
                perror("Erro de alocação de memória\n");
                return 1;
            }
        }

        // Argc = Contador de argumentos
        int argc = 0;

        // Finding current directory
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) // salva em cwb o path atual
        {
            perror("getcwd() error");
            return 1;
        }

        // Printando o prompt
        // \033[1;34mtexto colorido aqui\033[0m
        // Lendo linha de comando
        printf("\033[1;35mTaishell:\033[0m\033[1;34m~%s\033[0m$ ", cwd);
        fflush(stdout);
        fgets(cmdline, 50, stdin);

        // Verfificando se nada foi digitado
        if (strcmp(cmdline, "\n") == 0)
        {
            continue;
        }

        // Removendo caracter \n do final da linha
        cmdline[strlen(cmdline) - 1] = '\0';

        // Estrutura para criação de um vetor argv com todos os argumantos do comando lido
        // argc inicializado com 0
        argv[argc] = strtok(cmdline, " ");
        while (argv[argc] != NULL && argc < MAX_ARGV_SIZE - 1)
        {
            if (strcmp(argv[argc], ">") == 0)
            {
                redirectionIndex = argc;
            }
            argv[++argc] = strtok(NULL, " "); // argv vai ser um vetor de strings guardando todos os argumentos
        }
        argv[argc] = NULL;

        // Verificando comando exit
        if (strcmp(argv[0], "exit") == 0)
        {
            break;
        }

        // Verifiando comando cd
        if (strcmp(argv[0], "cd") == 0)
        {
            exec_cd(argv[1]);
            continue;
        }

        // Chamando função que cria um processo filho e executa o comando
        exec_Child(argc, argv, redirectionIndex);
    }
    return 0;
}

void handler(int signal)
{
    int status;
    while (waitpid(0, &status, WNOHANG) > 0)
        ;
}

void exec_Child(int argc, char **argv, int redirectionIndex)
{
    // Instanciação de variaveis
    pid_t pid;
    int status;
    char *outFile;

    // Variaveis de condição
    int issleep, isbackground;
    issleep = strcmp(argv[0], "sleep") == 0;
    isbackground = strcmp(argv[argc - 1], "&") == 0;

    // Variáveis de condição, vão ser usadas para implementar > e |
    int pipe_flag = 0;

    pid = fork();

    if (pid == -1)
    {
        perror("Erro na criação de processo");
        exit(1);
    }

    if (pid == 0) // Filho
    {

        // Se o carcter > for encontrado será feito um redirecionamento
        if (redirectionIndex > 0)
        {
            outFile = argv[redirectionIndex + 1];
            argv[redirectionIndex] = NULL;
            argv[redirectionIndex + 1] = NULL;

            int fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd == -1)
            {
                perror("Failed to open output file");
                return;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Por problemas ao rodar sleep pelo execvp, criei uma estrutura para sleep separadamente
        if (issleep)
        {
            sleep(atoi(argv[1]));
            exit(0);
            return;
        }
        if (execvp(argv[0], argv) == -1)
        {
            perror("error");
            fflush(stderr);
        }
        exit(1);
    }
    else // Pai
    {
        if (isbackground) // Chamada de background
        {
            signal(SIGCHLD, handler);
        }
        else // Chamada em foreground
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
    else if (strcmp(argv, "-") == 0)
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
