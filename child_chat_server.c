#include "child_chat_server.h"
#include "utils.h"
#include "command.h"
#include <signal.h>
#include <sys/socket.h>


int sockfd;
int pfd[2]; // write to parent & read from parent
int g_client_id = 0;

int run_child_process(int clientSocket, int ppid, int client_id, int pwrite_fd, int cread_fd){
    pfd[0] = cread_fd;  // read from parent
    pfd[1] = pwrite_fd; // write to parent
    char cmd_str[BUFSIZ];
    char strBuf[BUFSIZ];
    g_client_id = client_id;
    dprint("client[%d] start!!\n", g_client_id);
    sockfd=clientSocket;
    signal(SIGUSR2,sigusr2_handler);
    do{
        memset(cmd_str,0,BUFSIZ);
        memset(strBuf,0,BUFSIZ);
        int n = read(clientSocket,strBuf,BUFSIZ);
        if(n<=0){
            perror("read()");
            eprint("read 실패!!! \n");
            kill(ppid,SIGCHLD);
            return -1;
        }
        struct command_s tmp_cmd;
        memset(tmp_cmd.str1,0,CMD_STR1_SIZE);
        memset(tmp_cmd.str2,0,BUFSIZ);
        unpack_command(&tmp_cmd, strBuf);
        tmp_cmd.cid = g_client_id;
        pack_command(tmp_cmd, cmd_str, BUFSIZ);
        write(pfd[1],cmd_str,BUFSIZ);
        kill(ppid,SIGUSR1);
    }while(!stop);
    return 0;
}

void sigusr2_handler(){
    char command[BUFSIZ];
    int n,size;
    memset(command, 0, BUFSIZ);
    if((n=read(pfd[0], command, BUFSIZ))>0){
        dprint("sigusr2 : cmd(%s)\n",command);
        struct command_s cmd;
        memset(cmd.str1, 0, CMD_STR1_SIZE);
        memset(cmd.str2, 0, BUFSIZ);
        unpack_command(&cmd, command);
        dprint("cid(%d), fn1(%d), fn2(%d), str1(%s), str2(%s)\n",
                cmd.cid, cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
        
        if(cmd.func_num1 == CMD_CHANGE_ID){
            g_client_id = cmd.cid;
            return;
        }
        if(size = send(sockfd, command, BUFSIZ, MSG_DONTWAIT)<=0){
            perror("send()");
        }
    }
}