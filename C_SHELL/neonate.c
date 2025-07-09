#include "neonate.h"
#include <dirent.h>
#include <ctype.h>

void neonate(char *input)
{
    int wait_time = -1;
    char *token = (char *)malloc(sizeof(char) * buf_size);
    token = strtok(input, " \t");
    int i = 0;
    while (token != NULL)
    {
        if (i == 2)
        {
            wait_time = atoi(token);
        }
        i++;
        token = strtok(NULL, " \t");
    }
    printf("%d\n", wait_time);
    callingProcess(wait_time);
}


int is_key_pressed()
{
    struct termios oldt, newt;//two terminal control structures-oldt: stores the original terminal settings
    //newt: will be modified for custom behavior
    int ch;//ch stores the character read from input
    int oldf;//oldf stores the original file descriptor flags for stdin

    tcgetattr(STDIN_FILENO, &oldt);//Gets the current terminal attributes for standard input (STDIN_FILENO = 0) and stores them in oldt.
    newt = oldt;//Copies the original settings into newt so we can modify it safely.



    newt.c_lflag &= ~(ICANON | ECHO);//Turns off canonical mode and echo:ICANON: disables line-buffering (so we don't need Enter),ECHO: disables automatic echoing of typed characters
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);//Applies the modified settings immediately (without waiting).

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);//Gets the current file status flags (like blocking/non-blocking mode) for stdin.
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();//Attempts to read a character. If no key has been pressed, this returns EOF immediately.

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);//Restores the terminal to its original mode and blocking behavior.



    if (ch == EOF)
    {
        return 0;
    }

    return ch == 'x';
}
//Temporarily makes the terminal non-blocking and disables line buffering and echo.
//Checks whether a key was pressed.
//Returns 1 only if the key 'x' was pressed — without requiring Enter.
//Otherwise returns 0.
void executeNeonate()
{
    struct dirent *d;//proc directory — a special virtual filesystem in Linux that contains info about all running processes.
    DIR *Di = opendir("/proc");// the opendir() function is used to open a directory stream, which allows you to read the contents of a directory (like ls does).
    if (Di == NULL)
    {
        printf("Error in opening proc directory\n");
        return;
    }
    FILE *fp = fopen("/proc/loadavg", "r"); //Opens the file /proc/loadavg, which contains load average data and also the most recently created PID as its last field.

    if (fp == NULL)
    {
        printf("Error in opening loadavg file\n");
        return;
    }
    char *line = (char *)malloc(sizeof(char) * 1000);
    while (fgets(line, 1000, fp) != NULL)
    {
        char *token = strtok(line, " ");
        char *last_token = (char *)malloc(sizeof(char) * 100);
        while (token != NULL)
        {
            strcpy(last_token, token);
            token = strtok(NULL, " ");
        }
        int curr_pid = atoi(last_token);//PID of the most recently created process.

        printf("%d\n", curr_pid);
        free(last_token);
    }
    free(line);
}
struct termios orig_termios;

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void enableRawMode()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void callingProcess(int wait_time)
{
    enableRawMode();
    while (1)
    {
        if (is_key_pressed()) // (1) before executing anything
        {
            disableRawMode(); // disables canonical input & echo
            break;
        }
        executeNeonate(); // prints the newest PID
        if (is_key_pressed())// (2) after printing, before sleeping
        {
            disableRawMode();
            break;
        }
        sleep(wait_time);  // wait
        if (is_key_pressed()) // (3) after waiting
        {
            disableRawMode();
            break;
        }
    }
}