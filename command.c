#include "command.h"

int pack_command(struct command_s cmd, char *cmd_str, int cmd_str_size){
    int ret = snprintf(cmd_str,cmd_str_size,"%d%c%d%c%d%c%s%c%s%c",
                        cmd.cid, 
                        CMD_ETX, 
                        cmd.func_num1,
                        CMD_ETX, 
                        cmd.func_num2,
                        CMD_ETX,
                        cmd.str1,
                        CMD_ETX,
                        cmd.str2,
                        CMD_ETX);
    // vprint("cmd_str(%s)\n",cmd_str);
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
    // vprint("cid = %s\n",tmp);
    cmd->cid = atoi(tmp);

    // func num1 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(tmp, cmd_index, len);
    // vprint("func_num1 = %s\n",tmp);
    cmd->func_num1 = atoi(tmp);

    // func num2 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(tmp, cmd_index, len);
    // vprint("func_num2 = %s\n",tmp);
    cmd->func_num2 = atoi(tmp);
    

    // str1 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(cmd->str1, cmd_index, len);
    // vprint("str1 = %s\n",cmd->str1);

    // str2 추출
    cmd_index = end+1;
    end = strchr(cmd_index,CMD_ETX);
    if(!end){
        return -1;
    }
    len = end-cmd_index;
    strncpy(cmd->str2, cmd_index, len);
    // vprint("str2 = %s\n",cmd->str2);

    return 0;
}
