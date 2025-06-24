#ifndef __CHAT_SERVER_H__
#define __CHAT_SERVER_H__
#include "function.h"
#define MAX_NAME 100
#define MAX_USER 64
#define MAX_ROOM 64
#define MAX_PIPE 32

struct client_s {
    int id;
    int pid;
    int pipe;
    char name[MAX_NAME];
    char pass[MAX_NAME];
    int roomId;  // 0=lobby 1~=room_id
};

struct room_s {
    int id;
    int type;           // 0 = 일반 , 1 = 비밀방 , 2 = 1대1
    struct client_s *clients[MAX_USER];  // 방에 접속 중인 사용자의 포인터들
    int nclient;
    char name[MAX_NAME];
    char pass[MAX_NAME];
};

int stop = 0;
int nclient=0;
int room_n=0;
int sockfd;
struct client_s clients[MAX_USER];
struct room_s rooms[MAX_ROOM];
int pfd[2];
int cpfd[2];

int find_room_by_id(struct room_s rooms[MAX_ROOM], int id);
int find_room_by_name(struct room_s rooms[MAX_ROOM], char room_name[100]);
int broadcastChat(struct client_s *from_client, struct room_s rooms[MAX_ROOM], struct command_s cmd);
void signalStop();
int send_command(int fd, int pid, struct command_s cmd);
int find_empty_room_index(struct room_s rooms[MAX_ROOM]);
int func(char *command);
void signalUsr1Handler();
void signalUsr2Handler();


#endif //__CHAT_SERVER_H__