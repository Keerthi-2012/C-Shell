#include "dispReq.h"

void trimWhitespaces(char *str) // eliminates whitespaces from the string
{
    char *end;
    // removes whitespaces from front
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return;
    // removes whitespaces from back
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    *(end + 1) = '\0';
}

void print_The_Directory(char *home_dir)
{
    char *dir = (char *)malloc(sizeof(char) * buf_size);
    char *user_name = (char *)malloc(sizeof(char) * buf_size);
    struct utsname os; // a struct used to store information about operating system and machine
    int x = 0;
    int flag = 0;
    if (getlogin_r(user_name, buf_size) != 0)
    {
        char *env_user = getenv("USER");
        if (env_user)
            strncpy(user_name, env_user, buf_size);
        else
            strcpy(user_name, "user"); // last resort
    } // gets the username logged into os and stores in user_name
    getcwd(dir, buf_size); // gets current working directory and stores in dir variable
    uname(&os);            // fills the details of os in os struct

    char *filename = (char *)malloc(sizeof(char) * buf_size);
    strcpy(filename, home_dir);
    strcat(filename, "/test.txt");
    FILE *fp = fopen(filename, "r");
    char *line = (char *)malloc(sizeof(char) * buf_size);
    fgets(line, buf_size, fp);
    if (strcmp(line, "1\n") == 0) // a bit map to indicate whether a process (previous) took more than 2 secs or not
    {
        flag = 1; // flag will be set to 1 if it took
    }
    char *rel_dir = (char *)malloc(sizeof(char) * buf_size);

    if (strncmp(dir, home_dir, strlen(home_dir)) == 0)
    {
        if (strlen(dir) == strlen(home_dir))
        {
            strcpy(rel_dir, "~");
        }
        else
        {
            sprintf(rel_dir, "~%s", dir + strlen(home_dir));
        }
    }
    else
    {
        strcpy(rel_dir, dir); // absolute path
    }

    // Optional: handle the process that took >2s
    char *test_file = (char *)malloc(sizeof(char) * buf_size);
    sprintf(test_file, "%s/test.txt", home_dir);
    if (fp && fgets(line, buf_size, fp) && strcmp(line, "1\n") == 0)
    {
        flag = 1;
    }

    // Printing prompt
    printf("\033[0;35m<%s@%s", user_name, os.sysname);
    printf("\033[0;33m:%s", rel_dir);
    if (flag && fgets(line, buf_size, fp))
    {
        printf(":%s", line);
        fclose(fp);
        fp = fopen(test_file, "w");
    }
    printf("> \033[0m");
}