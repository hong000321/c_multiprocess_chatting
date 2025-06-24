#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>


#define DEBUG (4*1+3)
#include "utils.h"
#include "function.h"

int pfd[2];
int stop = 0;
void signalStop(){
    stop = 1;
}


int updateClient(struct command_s *cmd){
    dprint("update client page!!\n");
    switch (cmd->func_num1)
    {
    case CMD_SEND_ROOM:
        break;
    case CMD_ROOM_ADD:
        iprint("%s\n", cmd->str1);
        break;
    case CMD_ROOM_JOIN:
        iprint("%s\n", cmd->str1);
        break;
    case CMD_ROOM_LEAVE:
        iprint("%s\n", cmd->str1);
        break;
    case CMD_ROBBY_LOGIN:
        iprint("%s\n", cmd->str1);
        break;
    default:
        break;
    }
    return 0;
}


void signalUsr1Handler(){
    char cmd_str[BUFSIZ];
    struct command_s cmd;
    if(read(pfd[0], cmd_str, BUFSIZ) < 0){
        perror("recv()");
        eprint("signalUsr1Handler recv() error\n");
        return;
    }else{
        memset(cmd.str1, 0 ,CMD_STR1_SIZE);
        memset(cmd.str2, 0 ,BUFSIZ);
        
        unpack_command(&cmd,cmd_str);
        updateClient(&cmd);
        dprint("receivData : | fn1(%2d)  | fn2(%2d) | str1(%10s) | str2(%s)\n",cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
        iprint("%10s : %s\n",cmd.str1, cmd.str2);
    }
}

int sendData(int sockfd, char *mesg, int buf_size){
    char cmd_str[BUFSIZ];
    
    struct command_s cmd;
    memset(cmd_str, 0 ,BUFSIZ);
    int size = 0;
    for(int i=0; i<buf_size ; i++){
        if(mesg[i]=='\n' || mesg[i]==0){
            size = i;
            break;
        }
    }
    mesg[size] = '\0';
    dprint("sendData size = %d\n",size);
    if(size==0){
        dprint("sendData size is zero\n");
        return 0;
    }
    cmd.func_num1 = CMD_SEND_ROOM;
    char *mesg_index = mesg;
    
    strcpy(cmd.str1,"null");
    if(mesg[0]=='/'){
        mesg_index++;
        if(!strncmp((mesg_index), "add", 3)){
            cmd.func_num1 = CMD_ROOM_ADD;
            mesg_index+=4;
        }else if(!strncmp((mesg_index), "rm", 2)){
            cmd.func_num1 = CMD_ROOM_RM;
            mesg_index+=3;
        }else if(!strncmp((mesg_index), "login", 5)){
            cmd.func_num1 = CMD_ROBBY_LOGIN;
            mesg_index+=6;
            char *end = strchr(mesg_index,' ');
            memset(cmd.str1, 0, CMD_STR1_SIZE);
            strncpy(cmd.str1, mesg_index, end-mesg_index);
            mesg_index = end+1;
        }else if(!strncmp((mesg_index), "join", 4)){
            cmd.func_num1 = CMD_ROOM_JOIN;
            mesg_index+=5;
        }else if(!strncmp((mesg_index), "leave", 5)){
            cmd.func_num1 = CMD_ROOM_LEAVE;
            mesg_index+=6;
        }else if(!strncmp((mesg_index), "list", 4)){
            cmd.func_num1 = CMD_ROOM_LIST;
            mesg_index+=5;
        }else if(!strncmp((mesg_index), "users", 5)){
            cmd.func_num1 = CMD_ROOM_USERS;
            mesg_index+=6;
        }else{
            dprint("unkown command!!!(%s)\n",mesg);
            return -1;
        }
    }else if(mesg[0]=='!'){
        mesg_index++;
        if(!strncmp((mesg_index), "whisper", 7)){
            cmd.func_num1 = CMD_SEND_WHISPER;
            mesg_index+=8;
            char *end = strchr(mesg_index,' ');
            memset(cmd.str1, 0, CMD_STR1_SIZE);
            strncpy(cmd.str1, mesg_index, end-mesg_index);
            mesg_index = end+1;
        }
    }
    strcpy(cmd.str2,mesg_index);
    
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
        sendData(sockfd,strBuf,BUFSIZ);
        
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
            updateClient(&cmd);
            dprint("로그인 성공!! %s\n",name);
            break;
        }else if(cmd.func_num2 == FAIL){
            dprint("이미 존재하는 사용자 입니다.\n");
            printf("%s\n",cmd.str1);
            continue;
        }else{
            dprint("패킷 오류!!\n");
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
    char *address = "192.168.2.95";//argv[1]
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
    iprint("Connect Net = %s:%d\n",address,port);

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
    ret = pipe(pfd);
    if(ret < 0){
        perror("pipe()");
        eprint("pipe 에러");
        return -1;
    }

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
                write(pfd[1],recv_str,BUFSIZ);
                kill(ppid,SIGUSR1);
            }
        }while(!stop);
    }else{
        //부모 프로세스
        signal(SIGUSR1,signalUsr1Handler);
        do{
            char mesg[BUFSIZ];
            memset(mesg, 0 ,BUFSIZ);
            fgets(mesg, BUFSIZ, stdin);
            ret = sendData(sockfd,mesg,BUFSIZ);
            if(ret < 0){
                break;
            }
        }while(!stop);
    }
    close(sockfd);
    return 0;
}