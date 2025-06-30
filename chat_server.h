#ifndef __CHAT_SERVER_H__
#define __CHAT_SERVER_H__
#include "command.h"


int nclient=0;

void sigstop_handler();
int createServerSocket(int argc, char **argv);

#endif //__CHAT_SERVER_H__