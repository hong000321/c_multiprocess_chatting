#ifndef __FUNCTION_H__
#define __FUNCTION_H__
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
    SUCCESS,            // 11
    CONTINUE,           // 12
    END,                // 13
    FAIL,               // 14
};

struct command_s{
    int client_id;
    enum command_e func_num1;
    enum command_e func_num2;
    char str1[CMD_STR1_SIZE];
    char str2[BUFSIZ];
};

int pack_command(struct command_s cmd, char *cmd_str, int cmd_str_size){
    int ret = snprintf(cmd_str,cmd_str_size,"%d%c%d%c%d%c%s%c%s%c",
                        cmd.client_id, 
                        CMD_ETX, 
                        cmd.func_num1,
                        CMD_ETX, 
                        cmd.func_num2,
                        CMD_ETX,
                        cmd.str1,
                        CMD_ETX,
                        cmd.str2,
                        CMD_ETX);
    vprint("cmd_str(%s)\n",cmd_str);
    return ret;
}

int unpack_command(struct command_s *cmd, char *cmd_str){
    char tmp[100];
    memset(tmp,0,100);

    // client id 추출
    char *cmd_index = cmd_str;
    char *end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    int len = end-cmd_index;
    strncpy(tmp, cmd_index, len);
    vprint("client_id = %s\n",tmp);
    cmd->client_id = atoi(tmp);

    // func num1 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(tmp, cmd_index, len);
    vprint("func_num1 = %s\n",tmp);
    cmd->func_num1 = atoi(tmp);

    // func num2 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(tmp, cmd_index, len);
    vprint("func_num2 = %s\n",tmp);
    cmd->func_num2 = atoi(tmp);
    

    // str1 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(cmd->str1, cmd_index, len);
    vprint("str1 = %s\n",cmd->str1);

    // str2 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(cmd->str2, cmd_index, len);
    vprint("str2 = %s\n",cmd->str2);

    return 0;
}

#endif //__FUNCTION_H__