#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>


//#define DEBUG (4*1+3)
#include "utils.h"
#include "command.h"
int stop=0;
int ppfd[2];
void sigstop_handler(){
    stop = 1;
}


int update_client(struct command_s *cmd){
    dprint("update client page!!\n");
    switch (cmd->func_num1)
    {
    case CMD_ROOM_SEND:
        printf("\n%s : %s\n\n", cmd->str1, cmd->str2);
        break;
    case CMD_ROOM_ADD:
        printf("\n%s: %s\n\n", cmd->str1, cmd->str2);
        break;
    case CMD_ROOM_JOIN:
        printf("\n%s: %s\n\n", cmd->str1, cmd->str2);
        break;
    case CMD_ROOM_LEAVE:
        printf("\n%s\n\n", cmd->str1);
        break;
    case CMD_ROBBY_LOGIN:
        dprint("\n%s\n\n", cmd->str1);
        break;
    case CMD_ROOM_LIST:
        printf("\n%s\n%s\n\n", cmd->str1, cmd->str2);
        break;
    case CMD_ROOM_USERS:
        printf("\n%s\n%s\n\n", cmd->str1, cmd->str2);
        break;
    case CMD_WHISPER:
        printf("\n%s\n\n", cmd->str2);
        break;
    case CMD_ROOM_HELP:
        printf("\n%s\n%s\n\n", cmd->str1, cmd->str2);
        break;
    case CMD_EXIT:
        printf("exit command\n");
        stop = 1;
        break;
    default:
        break;
    }
    return 0;
}


void sigusr1_handler(){
    char cmd_str[BUFSIZ];
    struct command_s cmd;
    if(read(ppfd[0], cmd_str, BUFSIZ) < 0){
        perror("recv()");
        eprint("sigusr1_handler recv() error\n");
        return;
    }else{
        memset(cmd.str1, 0 ,CMD_STR1_SIZE);
        memset(cmd.str2, 0 ,BUFSIZ);
        
        unpack_command(&cmd,cmd_str);
        update_client(&cmd);
        dprint("receivData : | fn1(%2d)  | fn2(%2d) | str1(%10s) | str2(%s)\n",cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
        // dprint("%10s : %s\n",cmd.str1, cmd.str2);
    }
}

int send_data(int sockfd, char *mesg, int buf_size){
    char cmd_str[BUFSIZ];
    struct command_s cmd;
    int arg1 = 0;
    int size = 0;
    memset(cmd_str, 0 ,BUFSIZ);
    for(int i=0; i<buf_size ; i++){
        if(mesg[i]=='\n' || mesg[i]==0){
            size = i;
            break;
        }
    }
    mesg[size] = '\0';
    dprint("send_data size = %d\n",size);
    if(size==0){
        dprint("send_data size is zero\n");
        return 0;
    }
    cmd.func_num1 = CMD_ROOM_SEND;
    char *mesg_index = mesg;
    
    strcpy(cmd.str1,"null");
    if(mesg[0]=='/'){
        mesg_index++;
        if(!strncmp((mesg_index), "help", 4)){
            cmd.func_num1 = CMD_ROOM_HELP;
        }else if(!strncmp((mesg_index), "add", 3)){
            cmd.func_num1 = CMD_ROOM_ADD;
            arg1 = 1;
        }else if(!strncmp((mesg_index), "rm", 2)){
            cmd.func_num1 = CMD_ROOM_RM;
            arg1 = 1;
        }else if(!strncmp((mesg_index), "login", 5)){
            cmd.func_num1 = CMD_ROBBY_LOGIN;
            arg1 = 1;
        }else if(!strncmp((mesg_index), "join", 4)){
            cmd.func_num1 = CMD_ROOM_JOIN;
            arg1 = 1;
        }else if(!strncmp((mesg_index), "leave", 5)){
            cmd.func_num1 = CMD_ROOM_LEAVE;
        }else if(!strncmp((mesg_index), "list", 4)){
            cmd.func_num1 = CMD_ROOM_LIST;
        }else if(!strncmp((mesg_index), "users", 5)){
            cmd.func_num1 = CMD_ROOM_USERS;
        }else if(!strncmp((mesg_index), "exit", 4)){
            cmd.func_num1 = CMD_EXIT;
        }else{
            eprint("unkown command!!!(%s)\n",mesg);
            return -1;
        }
    }else if(mesg[0]=='!'){
        mesg_index++;
        if(!strncmp((mesg_index), "whisper", 7)){
            cmd.func_num1 = CMD_WHISPER;
            arg1 = 1;
        }else{
            eprint("unkown command!!!(%s)\n",mesg);
            return -1;
        }
    }
    char *end = strchr(mesg_index,' ');
    if(end){
        mesg_index = end+1;
    }
    if(arg1){
        dprint("arg1 command : %s\n",mesg_index);
        memset(cmd.str1, 0, CMD_STR1_SIZE);
        end = strchr(mesg_index,' ');
        if(((cmd.func_num1 == CMD_ROOM_ADD) || (cmd.func_num1 == CMD_ROOM_JOIN)) && !end){
            strcpy(cmd.str1, mesg_index);
            goto pack_cmd;
        }
        if(!end){
            eprint("command error!!!\n");
            return -1;
        }
        strncpy(cmd.str1, mesg_index, end-mesg_index);
        mesg_index = end+1;
        strcpy(cmd.str2,mesg_index);
    }else{
        strcpy(cmd.str1,mesg_index);
    }
pack_cmd:
    pack_command(cmd, cmd_str, BUFSIZ);
    if(size = send(sockfd, cmd_str, BUFSIZ, MSG_DONTWAIT)<=0){
        perror("send()");
        return -1;
    }
    return size;
}

int requestLogin(int sockfd){
    char strBuf[BUFSIZ];
    char name[BUFSIZ];
    char pass[BUFSIZ];
    
    int ret=1;
    struct command_s cmd;

    do{
        memset(cmd.str1,0,CMD_STR1_SIZE);
        memset(cmd.str2,0,BUFSIZ);
        memset(strBuf,0, BUFSIZ);
        memset(name,0, BUFSIZ);
        memset(pass,0, BUFSIZ);
        printf("사용자명과 비밀번호를 입력하세요 :\n");
        scanf("%s %s",name,pass);

        snprintf(strBuf,BUFSIZ,"/login %s %s",name, pass);
        send_data(sockfd,strBuf,BUFSIZ);
        
        memset(strBuf,0, BUFSIZ);
        if(recv(sockfd, strBuf, BUFSIZ, 0) < 0){
            perror("recv()");
            eprint("requestLogin recv() error\n");
            return -1;
        }else{
            unpack_command(&cmd,strBuf);
            dprint("receivData : | fn1(%2d)  | fn2(%2d) | str1(%10s) | str2(%s)\n",cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
        }
        if(cmd.func_num2 == SUCCESS){
            update_client(&cmd);
            printf("로그인 성공!! %s\n",name);
            printf("현재 위치: %s\n",cmd.str1);
            break;
        }else if(cmd.func_num2 == FAIL){
            dprint("이미 존재하는 사용자 입니다.\n");
            printf("%s\n",cmd.str1);
            continue;
        }else{
            eprint("패킷 오류!!\n");
        }
    }while(1);
    
    return 0;
}


int main(int argc, char **argv){
    int ret = -1;
    int sockfd , n;
    socklen_t clisize;
    struct sockaddr_in servaddr, cliaddr;
    char command[BUFSIZ];
    char *address = "127.0.0.1";//"192.168.2.95";//argv[1]
    int port;
    
    
    if(argc == 2){
        port = atoi(argv[1]);
    }else if(argc == 1){
        port = 5101;
    }else if(argc == 3){
        address = argv[1];
        port = atoi(argv[2]);
    }else{
        iprint("usage : %s <Port>\n", argv[0]);
        iprint("usage : %s <IP address> <Port>\n", argv[0]);
        return -1;
    }

    // 소켓 생성
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("socket()");
        return -1;
    }
    
    // 소켓이 접속할 주소 지정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    inet_pton(AF_INET, address, &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(port);
    iprint("Try Connect Net = %s:%d\n",address,port);

    //지정한 주소로 접속
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0){
        perror("connect()");
        return -1;
    }
    // 로그인 진행
    ret = requestLogin(sockfd);
    if(ret < 0){
        dprint("로그인 실패!!! \n");
        return -1;
    }

    // 파이프 생성
    ret = pipe(ppfd);
    if(ret < 0){
        perror("pipe()");
        eprint("pipe 에러");
        return -1;
    }
    signal(SIGINT,sigstop_handler);
    signal(SIGTERM,sigstop_handler);

    pid_t pid;
    if((pid=fork())<0){
        perror("fork()");
        return -1;
    }else if(pid == 0){
        //자식 프로세스
        pid_t ppid = getppid();
        do {
            char recv_str[BUFSIZ];
            if(recv(sockfd, recv_str, BUFSIZ, 0) < 0){
                perror("recv()");
                return -1;
            }else{
                write(ppfd[1],recv_str,BUFSIZ);
                kill(ppid,SIGUSR1);
            }
        }while(!stop);
    }else{
        //부모 프로세스
        char mesg[BUFSIZ];
        signal(SIGUSR1,sigusr1_handler);
        signal(SIGCHLD,SIG_IGN);
        do{
            memset(mesg, 0 ,BUFSIZ);
            fgets(mesg, BUFSIZ, stdin);
            ret = send_data(sockfd,mesg,BUFSIZ);
            if(ret < 0){
                eprint("send_data() error\n");
                kill(pid, SIGINT);
                break;
            }
        }while(!stop);
        kill(pid, SIGINT);
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if(pid < 0){
            perror("waitpid()");
            eprint("waitpid() error\n");
        }else if(pid == 0){
            dprint("no child process\n");
        }else{
            dprint("child process %d terminated\n", pid);
        }
    }
    close(sockfd);
    close(ppfd[0]);
    close(ppfd[1]);
    return 0;
}