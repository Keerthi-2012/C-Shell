#ifndef INPUTREQ_H
#define INPUTREQ_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "activities.h"
#define buf_size 4096
void dividingCommands(char *input, char *home_dir, char *logFileName, char *previous_directory);
int processCommands(char *input);
#endif