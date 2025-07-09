#include "pipes_redirection.h"
#include "activities.h"
void both(char *input, char *home_dir, char *previous_dir, char *log_filename,int fgbg)
{
    int n = numberOfPipes(input);
    int pipe_fd[n][2];
    char *token = strtok(input, "|");
    int i = 0;

    int print_directory = 1;

    while (token)
    {
        if (i < n && pipe(pipe_fd[i]) < 0)//The pipe() system call is used in inter-process communication (IPC) to create a unidirectional communication channel between processes.
        {
            perror("Pipe Failed");
            exit(EXIT_FAILURE);
        }

        int pid = fork();
        if (pid < 0)
        {
            perror("Fork Failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            if (i > 0)
            {
                dup2(pipe_fd[i - 1][0], STDIN_FILENO);//system call to redirect stdin to pipe read end
            }

            if (i < n)
            {
                dup2(pipe_fd[i][1], STDOUT_FILENO);// redirect stdout to pipe write end
            }

            for (int j = 0; j < n; j++)
            {
                close(pipe_fd[j][0]);
                close(pipe_fd[j][1]);
            }
            processRedirection(token, home_dir, previous_dir, log_filename, fgbg);
            exit(0);
        }
        else
        {
            if (i > 0)
            {
                close(pipe_fd[i - 1][0]);
                close(pipe_fd[i - 1][1]);
            }
            token = strtok(NULL, "|");
            i++;
        }
    }

    for (i = 0; i < n; i++)
    {
        close(pipe_fd[i][0]);
        close(pipe_fd[i][1]);
    }

    for (i = 0; i <= n; i++)
    {
        wait(NULL);
    }
}