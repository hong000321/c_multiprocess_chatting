#ifndef PARENT_CHAT_SERVER_H
#define PARENT_CHAT_SERVER_H
#include "chat_server_global.h"


struct client_s {
    int id;    // 클라이언트 ID
    int pid;
    int pipe[2];       // 0: read from child ,  1: write to child
    char name[MAX_NAME];
    char pass[MAX_NAME];
    int roomId;  // 0=lobby 1~=room_id
};

struct room_s {
    int id;
    int type;           // 0 = 일반 , 1 = 비밀방 , 2 = 1대1
    struct client_s *clients[MAX_USER];  // 방에 접속 중인 사용자의 포인터들
    int nclients;
    char name[MAX_NAME];
    char pass[MAX_NAME];
};



void init_parent();
int run_parent_work(int clientPid, int client_id, int pread_fd, int cwrite_fd);
void sigusr1_handler(int signo, siginfo_t *info, void *context);

#endif // PARENT_CHAT_SERVER_H