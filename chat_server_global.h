#ifndef CHAT_SERVER_GLOBAL_H
#define CHAT_SERVER_GLOBAL_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_NAME 100
#define MAX_USER 64
#define MAX_ROOM 64
#define MAX_PIPE 32
#define READ 0
#define WRITE 1


extern int stop;

#endif // CHAT_SERVER_GLOBAL_H