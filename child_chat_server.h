#ifndef CHILD_CHAT_SERVER_H
#define CHILD_CHAT_SERVER_H
#include "chat_server_global.h"

int run_child_process(int clientSocket, int ppid, int client_id, int pwrite_fd, int cread_fd);
void sigusr2_handler();


#endif //CHILD_CHAT_SERVER_H