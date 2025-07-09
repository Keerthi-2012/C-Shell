#include "FG_BG.h"
#include "signals_.h" // Changed from <signals_.h> to "signals_.h"
extern int top;


void clean_dead_activities()
{
    for (int i = 0; i < top; i++)
    {
        if (activities[i].pid != -1)
        {
            if (kill(activities[i].pid, 0) == -1 && errno == ESRCH)
            {
                activities[i].pid = -1;
                activities[i].fg_bg = -1;
            }
        }
    }
}

void GetID_FG_BG(char *input, char *home_dir)
{
    int i = 0;
    int fg_or_bg = -1;
    int id = -1;

    char *token = strtok(input, " \t");
    while (token != NULL)
    {
        if (i == 0)
        {
            if (strcmp(token, "fg") == 0)
                fg_or_bg = 1;
            else if (strcmp(token, "bg") == 0)
                fg_or_bg = 0;
            i++;
        }
        else if (i == 1)
        {
            id = atoi(token);
            i++;
        }
        token = strtok(NULL, " \t");
    }

    if (id == -1 || fg_or_bg == -1)
    {
        printf("Invalid usage of fg/bg\n");
        return;
    }

    executeFG_BG(id, fg_or_bg, home_dir);
}

void executeFG_BG(int pid, int is_fg, char *home_dir)
{
    if (pid <= 0)
    {
        printf("Invalid PID\n");
        return;
    }

    // Check if process exists
    if (kill(pid, 0) == -1 && errno == ESRCH)
    {
        printf("No such process found\n");
        return;
    }

    // Check if PID is tracked in activities[]
    int found = 0;
    for (int i = 0; i < top; i++)
    {
        if (activities[i].pid == pid)
        {
            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("No such process found\n");
        return;
    }
    // It tells a stopped (paused) process to resume execution.-SIGCNT
    //Sends the SIGCONT signal to the process pid.-kill(pid, SIGCONT)
    // Resume the process
    if (kill(pid, SIGCONT) == -1)
    {
        perror("Failed to send SIGCONT");
        return;
    }

    // Print info based on fg/bg
    if (is_fg)
    {
        printf("Bringing process [%d] to foreground\n", pid);

        // Give control of terminal to process
        tcsetpgrp(STDIN_FILENO, pid);

        int status;
        waitpid(pid, &status, WUNTRACED);

        // Restore terminal control back to shell
        tcsetpgrp(STDIN_FILENO, getpgrp());

        if (WIFEXITED(status))
        {
            printf("Process [%d] exited with status %d\n", pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Process [%d] terminated by signal %d\n", pid, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            printf("Process [%d] was stopped\n", pid);
        }
    }
    else
    {
        printf("Resumed process [%d] in background\n", pid);
    }

    // Update activity state
    for (int i = 0; i < top; i++)
    {
        if (activities[i].pid == pid)
        {
            activities[i].fg_bg = is_fg;
            break;
        }
    }
}
