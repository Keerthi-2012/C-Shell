#include "hop.h"
#include "dispReq.h"


// function to hop to requested directory if it is valid
void hop(char *cmd, char *home_dir, char *prevDir)
{
    char curr_dir[buf_size];
    char temp_cmd[buf_size];
    strcpy(temp_cmd, cmd);

    temp_cmd[strcspn(temp_cmd, "\n")] = 0;
    trimWhitespaces(temp_cmd);

    char *token = strtok(temp_cmd, " \t"); // skip "hop"
    token = strtok(NULL, " \t");

    if (!token) {
        getcwd(prevDir, buf_size);
        chdir(home_dir);
        printf("%s\n", home_dir);
        return;
    }

    while (token != NULL) {
        char new_path[2 * buf_size]; // avoid truncation warnings
        getcwd(curr_dir, buf_size);

        if (strcmp(token, "~") == 0) {
            strcpy(new_path, home_dir);
        } else if (strcmp(token, "-") == 0) {
            strcpy(new_path, prevDir);
        } else if (strcmp(token, ".") == 0 || strcmp(token, "..") == 0) {
            strcpy(new_path, token);
        } else if (token[0] == '~') {
            // ~/xyz
            snprintf(new_path, sizeof(new_path), "%s%s", home_dir, token + 1);//string print with formatting
        } else if (token[0] == '/') {
            // absolute path
            strncpy(new_path, token, sizeof(new_path) - 1);
            new_path[sizeof(new_path) - 1] = '\0';
        } else {
            // relative path
            snprintf(new_path, sizeof(new_path), "%s/%s", curr_dir, token);
        }

        if (chdir(new_path) == 0) {
            strcpy(prevDir, curr_dir);
        } else {
            perror("Error:");
        }

        token = strtok(NULL, " \t");
    }

    getcwd(curr_dir, buf_size);
    printf("%s\n", curr_dir);
}
