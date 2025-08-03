#ifndef COMMAND_H
#define COMMAND_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define CMD_STR1_SIZE 100
#define CMD_ETX 0x03 // 문자열 구분을 위한 'end of text'
enum command_e{
    CMD_NULL,           // 0
    CMD_ROBBY_LOGIN,    // 1
    // ROOM
    CMD_ROOM_ADD,       // 2
    CMD_ROOM_RM,        // 3
    CMD_ROOM_JOIN,      // 4
    CMD_ROOM_LEAVE,     // 5
    CMD_ROOM_LIST,      // 6
    CMD_ROOM_USERS,     // 7
    // CHAT
    CMD_ROOM_SEND,      // 8
    CMD_WHISPER,        // 9
    // ETC
    CMD_EXIT,           // 10
    CMD_CHANGE_ID,      // 11  자식 프로세스의 ID를 변경하기 위한 커맨드, client에 전달되지는 않음
    SUCCESS,            // 12
    CONTINUE,           // 13
    END,                // 14
    FAIL,               // 15
    CMD_ROOM_HELP,      // 16
};

struct command_s{
    int cid; // client index
    enum command_e func_num1;
    enum command_e func_num2;
    char str1[CMD_STR1_SIZE];
    char str2[BUFSIZ];
};

int pack_command(struct command_s cmd, char *cmd_str, int cmd_str_size);
int unpack_command(struct command_s *cmd, char *cmd_str);


#endif // COMMAND_H