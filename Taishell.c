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
int exec_command(int argc, char **argv, int redirectionIndex, int pipeIndex);
int exec_cd(char *argv, char *lastdir);
void handler(int signal);

int main()
{
    // Variável para guardar o valor do ultimo diretório acessado inicializando com null
    char lastdir[PATH_MAX];

    while (1)
    {
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

        // Variaveis para redirecionamento de output e pipe
        int redirectionIndex = -1;
        int pipeIndex = -1;

        // Variável de diretório cwd: Current Working Directory
        char cwd[PATH_MAX];

        // Atualizando o ultimo diretorio acessado
        strcpy(lastdir, cwd);

        // Salvando o diretório atual
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

        // Estrutura para criação de um vetor argv com todos os argumentos do comando lido.
        // argc é inicializado com 0
        argv[argc] = strtok(cmdline, " ");
        while (argv[argc] != NULL && argc < MAX_ARGV_SIZE - 1)
        {
            if (strcmp(argv[argc], ">") == 0)
            {
                redirectionIndex = argc;
            }
            if (strcmp(argv[argc], "|") == 0)
            {
                pipeIndex = argc;
            }

            // argv vai ser um vetor de strings guardando todos os argumentos
            argv[++argc] = strtok(NULL, " ");
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
            exec_cd(argv[1], lastdir);
            continue;
        }

        // Chamando função que cria um processo filho e executa o comando
        exec_command(argc, argv, redirectionIndex, pipeIndex);

        for (int i = 0; i < MAX_ARGV_SIZE; i++)
        {
            free(argv[i]);
        }
        free(argv);
    }
    return 0;
}

void handler(int signal)
{
    int status;
    while (waitpid(0, &status, WNOHANG) > 0)
        ;
}

int exec_command(int argc, char **argv, int redirectionIndex, int pipeIndex)
{
    // Redirecionamento de output
    // Instanciação de variáveis
    pid_t result;
    int status;
    char *outFile;

    // Variáveis de condição
    int issleep, isbackground;
    issleep = strcmp(argv[0], "sleep") == 0;
    isbackground = strcmp(argv[argc - 1], "&") == 0;

    // Criando processo filho
    result = fork();

    if (result == -1)
    {
        perror("Erro na criação de processo");
        return 1; // Retorna código de erro
    }

    if (result == 0)
    { // Filho
        // Se o caractere "|" foi encontrado, será feito um pipe
        if (pipeIndex > 0)
        {
            // Array de descritores de arquivo
            int pipe_fds[2];

            // Criando os dois descritores de arquivo com a função pipe
            // pipe_fds[0] será lido
            // pipe_fds[1] será escrito
            if (pipe(pipe_fds) == -1)
            {
                perror("pipe");
                return 1; // Retorna código de erro
            }

            // Criação do segundo processo
            pid_t result2 = fork();
            if (result2 == -1)
            {
                perror("Erro na criação de processo");
                return 1; // Retorna código de erro
            }

            if (result2 == 0)
            { // Processo filho 2
                // Fechando o descritor de arquivo que não será usado
                close(pipe_fds[1]);

                // Mudando o descritor de arquivo padrão
                dup2(pipe_fds[0], STDIN_FILENO);

                // Criando argv para o segundo processo
                char *argv2[MAX_ARGV_SIZE];
                for (int i = 0; i < MAX_ARGV_SIZE; i++)
                {
                    argv2[i] = (char *)malloc(MAX_ARG_SIZE * sizeof(char));
                    if (argv2[i] == NULL)
                    {
                        perror("Erro de alocação de memória\n");
                        exit(1);
                    }
                }

                // Copiando os valores para argv2
                for (int i = pipeIndex + 1, j = 0; i < argc; i++, j++)
                {
                    strcpy(argv2[j], argv[i]);
                }
                argv2[argc - pipeIndex - 1] = NULL;

                int erroExec = execvp(argv2[0], argv2);
                if (erroExec == -1)
                {
                    perror("execvp");
                    exit(1);
                }
            }
            else
            { // Processo filho 1
                // Fechando o descritor de arquivo que não será usado
                close(pipe_fds[0]);

                // Mudando o descritor de arquivo padrão
                dup2(pipe_fds[1], STDOUT_FILENO);

                argv[pipeIndex] = NULL;
                int erroExec = execvp(argv[0], argv);
                if (erroExec == -1)
                {
                    perror("execvp");
                    exit(1);
                }
            }
        }

        // Se o caractere ">" for encontrado, será feito um redirecionamento
        if (redirectionIndex > 0 && redirectionIndex < argc - 1)
        {
            outFile = argv[redirectionIndex + 1];
            argv[redirectionIndex] = NULL;
            argv[redirectionIndex + 1] = NULL;

            int fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd == -1)
            {
                perror("Falha ao abrir o arquivo de saída");
                return 1; // Retorna código de erro
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Por problemas ao executar "sleep" pelo execvp, criou-se uma estrutura para sleep separadamente
        if (issleep)
        {
            sleep(atoi(argv[1]));
            exit(0);
        }

        // Executando processo filho sem pipe
        int erroExec = execvp(argv[0], argv);
        if (erroExec == -1)
        {
            perror("execvp");
            exit(1);
        }
    }
    else
    {                     // Pai
        if (isbackground) // Chamada em background
        {
            signal(SIGCHLD, handler); // Define sua função de tratamento
        }
        else // Chamada em primeiro plano
        {
            wait(&status);
        }
    }
    return 0; // Retorna sucesso
}

int exec_cd(char *argv, char *lastdir)
{
    char curdir[PATH_MAX];
    char path[PATH_MAX];

    // Verifica se é possível obter o diretório atual
    if (getcwd(curdir, sizeof curdir))
    {
        // O diretório atual pode ser inacessível (não é um erro)
        *curdir = '\0'; // Define a string vazia para indicar que o diretório atual não está disponível
    }

    // Verifica se o argumento é nulo
    if (argv == NULL)
    {
        // Se o argumento for nulo, muda para o diretório HOME do usuário
        argv = getenv("HOME");
    }
    else if (strcmp(argv, "-") == 0)
    {
        // Se o argumento for "-", muda para o diretório anterior
        if (*lastdir == '\0')
        {
            // Se o diretório anterior não estiver disponível, exibe uma mensagem de erro
            fprintf(stderr, "Nenhum diretório anterior disponível\n");
            return 1; // Retorna 1 para indicar um erro
        }
        argv = lastdir; // Define o diretório anterior como o destino
    }
    else if (*argv == '~')
    {
        // Se o argumento começar com "~"
        if (argv[1] == '/' || argv[1] == '\0')
        {
            // Se o próximo caractere for "/" ou se o restante da string for vazio,
            // expande para o diretório HOME do usuário seguido do restante do caminho
            snprintf(path, sizeof path, "%s%s", getenv("HOME"), argv + 1);
            argv = path; // Define o caminho expandido como o destino
        }
        else
        {
            // Se houver um nome de usuário após o "~", exibe uma mensagem de erro
            fprintf(stderr, "Sintaxe não suportada: %s\n", argv);
            return 1;
        }
    }

    // Tenta mudar para o diretório especificado
    if (chdir(argv))
    {
        fprintf(stderr, "%s: %s: %s\n", argv, strerror(errno), path);
        return 1;
    }

    // Atualiza o diretório anterior
    strcpy(lastdir, curdir);

    return 0;
}
