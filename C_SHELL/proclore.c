#include "proclore.h"

void proclore()
{
    pid_t processID = getpid(); // get the pid of process
    char proc_path[buf_size];
    FILE *status_file;
    char processStatus;
    // construct process path
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", processID); // Stores it in proc_path
    status_file = fopen(proc_path, "r");
    if (status_file == NULL)
        perror("\033[0;31m There is an error in opening the file \033[0\n");
    fscanf(status_file, "%*d %*s %c", &processStatus);
    // Reads and skips the first two fields (PID and executable name), then reads the third field, which is the process status:

    // Will store the virtual memory size (in bytes) used by the process.
    unsigned long virtual_memory;
    fscanf(status_file, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %lu", &virtual_memory);
    fclose(status_file);//Closes the /proc/<pid>/stat file after reading.


    // This long format skips over many fields in the /proc/<pid>/stat file to reach the 23rd field:
    // virtual_memory = vsize

    int processGroupID = getpgid(processID); // get group id of the process -Useful for job control and grouping related processes.


    // exe_path is needed to know where the symbolic link is. 
    // readlink(exe_path) to resolve that symlink and get the real executable path in exe_realpath
    char exe_path[256], exe_realpath[256];
    sprintf(exe_path, "/proc/%d/exe", processID);
    ssize_t len = readlink(exe_path, exe_realpath, sizeof(exe_realpath) - 1);//readlink() reads the symbolic link from /proc/<pid>/exe to get the actual path of the executable.Stores it in exe_realpath.


    if (len != -1)
    {
        exe_realpath[len] = '\0';
    }
    else
    {
        printf("\033[0;31mExecutable Path: Not found \033[0\n");
    }

    printf("pid: %d\n", processID);
    printf("process Status: %c\n", processStatus);
    printf("process Group: %d\n", processGroupID);
    printf("Virtual memory: %ld\n", virtual_memory);
    printf("Executable path: %s\n", exe_realpath);
}

void procloreID(int processID)
{
    char proc_path[buf_size];
    FILE *status_file;
    char processStatus;

    // Construct the path to the stat file
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", processID);

    // Open the stat file
    status_file = fopen(proc_path, "r");
    if (status_file == NULL)
    {
        perror("\033[0;31mError opening the file \033[0;31m");
        return;
    }

    // Read the process status from the stat file
    if (fscanf(status_file, "%*d %*s %c", &processStatus) != 1)
    {
        printf("\033[0;31mError reading process status\033[0;31m\n");
        fclose(status_file);
        return;
    }

    unsigned long virtual_memory;
    // Read the virtual memory from the stat file
    if (fscanf(status_file, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %lu", &virtual_memory) != 1)
    {
        printf("\033[0;31mError reading virtual memory\033[0\n");
        fclose(status_file);
        return;
    }
    fclose(status_file);

    int processGroupID = getpgid(processID);
    if (processGroupID == -1)
    {
        perror("\033[0;31mError getting process group ID\033[0");
        return;
    }

    char exe_path[256], exe_realpath[256];
    snprintf(exe_path, sizeof(exe_path), "/proc/%d/exe", processID);
    ssize_t len = readlink(exe_path, exe_realpath, sizeof(exe_realpath) - 1);
    printf("pid: %d\n", processID);
    printf("process Status: %c\n", processStatus);
    printf("process Group: %d\n", processGroupID);
    printf("Virtual memory: %ld\n", virtual_memory);
    if (len != -1)
    {
        exe_realpath[len] = '\0';
        printf("Executable path: %s\n", exe_realpath);
    }
    else
    {
        printf("\033[0;31mExecutable path : Not found\033[0\n");
    }
}