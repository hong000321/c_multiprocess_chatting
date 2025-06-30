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
    CMD_ROOM_ADD,       // 2
    CMD_ROOM_RM,        // 3
    CMD_ROOM_JOIN,      // 4
    CMD_ROOM_PASS,      // 5
    CMD_ROOM_LEAVE,     // 6
    CMD_ROOM_LIST,      // 7
    CMD_ROOM_USERS,     // 8
    CMD_SEND_ROOM,      // 9
    CMD_SEND_WHISPER,   // 10
    CMD_EXIT,           // 11
    CMD_CHANGE_ID,      // 12
    SUCCESS,            // 13
    CONTINUE,           // 14
    END,                // 15
    FAIL,               // 16
    CMD_ROOM_HELP,      // 17
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