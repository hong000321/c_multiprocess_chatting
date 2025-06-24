#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdlib.h>
#define DEBUG (4*1 + 3)
#include "chat_server.h"
#include "utils.h"
#include "function.h"

int main(int argc, char **argv){
    int mainSocket, clientSocket;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen;
    char mesg[BUFSIZ];
    pid_t broadcastPid, clientPid;
    int ppid = getpid();

    
    // 주소와 포트 설정
    int port = 5101;
    char *address = "192.168.2.95";
    if(argc == 2){
        port = atoi(argv[1]);
    }else if(argc == 1){
        // port = 5101;
    }else if(argc == 3){
        address = argv[1];
        port = atoi(argv[2]);
    }else{
        iprint("usage : %s <Port>\n", argv[0]);
        iprint("usage : %s <IP address> <Port>\n", argv[0]);
        return -1;
    }

    if((mainSocket=socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("socket()");
        return -1;
    }

    // 방 초기화
    for(int i=0; i<MAX_ROOM; i++){
        rooms[i].id = -1;
    }
    room_n++;
    
    memset(&servaddr, 0 , sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port        = htons(port);
    iprint("Server Net = %s:%d\n",address,port);
    
    if(bind(mainSocket, (struct sockaddr *)&servaddr, sizeof(servaddr))<0){
        perror("bind()");
        return -1;
    }

    if(listen(mainSocket, 8)<0){
        perror("listen");
        return -1;
    }

    // 파이프 생성
    int ret = pipe(pfd);
    if(ret < 0){
        perror("pipe()");
        return -1;
    }
    
    signal(SIGINT,signalStop);
    signal(SIGTERM,signalStop);
    signal(SIGUSR1,signalUsr1Handler);
    clen = sizeof(struct sockaddr_in);
    
    do{
        clientSocket = accept(mainSocket, (struct sockaddr*)&cliaddr, &clen);
        if(clientSocket > 0){
            if(pipe(cpfd)){
                perror("cpipe()");
                break;
            }
            if((clientPid=fork())==0){
                char cmd_str[BUFSIZ];
                char strBuf[BUFSIZ];
                dprint("client[%d] start!!\n", nclient);
                sockfd=clientSocket;
                signal(SIGUSR2,signalUsr2Handler);
                do{
                    memset(cmd_str,0,BUFSIZ);
                    memset(strBuf,0,BUFSIZ);
                    int n = read(clientSocket,strBuf,BUFSIZ);
                    if(n<=0){
                        perror("read()");
                        eprint("read 실패!!! \n");
                        break;
                    }
                    struct command_s tmp_cmd;
                    memset(tmp_cmd.str1,0,CMD_STR1_SIZE);
                    memset(tmp_cmd.str2,0,BUFSIZ);
                    unpack_command(&tmp_cmd, strBuf);
                    tmp_cmd.client_id = nclient;
                    pack_command(tmp_cmd, cmd_str, BUFSIZ);
                    write(pfd[1],cmd_str,BUFSIZ);
                    kill(ppid,SIGUSR1);
                }while(!stop);
            }else{
                clients[nclient].id = nclient;
                clients[nclient].pipe = cpfd[1];
                clients[nclient].roomId = 0;
                clients[nclient].pid = clientPid;
                nclient++;
                
            }
        }
    }while(!stop);


    return 0;
}



int find_room_by_id(struct room_s rooms[MAX_ROOM], int id){
    for(int i=0; i<MAX_ROOM ; i++){
        if(id == rooms[i].id){
            return i;
        }
    }
    return -1;
}

int find_room_by_name(struct room_s rooms[MAX_ROOM], char room_name[100]){
    for(int i=0; i<MAX_ROOM ; i++){
        if(!strcmp(rooms[i].name, room_name)){
            return i;
        }
    }
    return -1;
}

int broadcastChat(struct client_s *from_client, struct room_s rooms[MAX_ROOM], struct command_s cmd){
    if(from_client->roomId==0){
        eprint("is lobby\n");
        return -1;
    }
    int room_index = find_room_by_id(rooms,from_client->roomId);
    dprint("from(%d) room_index(%d), nclient(%d), strBuf(%s)\n",from_client->id,room_index, rooms[room_index].nclient, cmd.str2);
    struct command_s ret_command;
    char cmd_str[BUFSIZ];
    ret_command.client_id = from_client->id;
    ret_command.func_num1 = cmd.func_num1;
    ret_command.func_num2 = SUCCESS;
    strcpy(ret_command.str1,from_client->name);
    strcpy(ret_command.str2,cmd.str2);
    pack_command(ret_command,cmd_str,BUFSIZ);


    for(int i=0; i<rooms[room_index].nclient; i++){
        if(rooms[room_index].clients[i]->id == from_client->id){
            continue;
        }
        write(rooms[room_index].clients[i]->pipe,cmd_str,BUFSIZ);
        kill(rooms[room_index].clients[i]->pid, SIGUSR2);
    }
    return 0;
}

void signalStop(){
    stop = 1;
}

int send_command(int fd, int pid, struct command_s cmd){
    char cmd_str[BUFSIZ];
    memset(cmd_str, 0, BUFSIZ);
    pack_command(cmd,cmd_str,BUFSIZ);
    vprint("client_id(%d), fn1(%d), fn2(%d), str1(%s), str2(%s)\n", cmd.client_id, cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
    
    write(fd,cmd_str,BUFSIZ);
    kill(pid, SIGUSR2);
    return 0;
}

int find_empty_room_index(struct room_s rooms[MAX_ROOM]){
    for(int i=0; i<MAX_ROOM; i++){
        if(rooms[i].id==-1){
            return i;
        }
    }
    return -1;
}
// | client_id | func_num1 | func_num2 | input1 | input2 |
int func(char *command){
    struct command_s cmd;
    memset(cmd.str1, 0, CMD_STR1_SIZE);
    memset(cmd.str2, 0, BUFSIZ);
    
    int ret = unpack_command(&cmd,command);
    vprint("client_id(%d), fn1(%d), fn2(%d), str1(%s), str2(%s)\n", cmd.client_id, cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
    if(ret < 0){
        return -1;
    }
    struct command_s send_cmd;
    send_cmd.func_num1 = cmd.func_num1;
    memset(send_cmd.str1,0,100);
    memset(send_cmd.str2,0,BUFSIZ);
    int room_index;
    switch (cmd.func_num1)
    {
    case CMD_ROBBY_LOGIN:
#if LOGIN
            for(int i=0; i<nclient ; i++){
            if((!strcmp(clients[i].name, cmd.str1)) && (!strcmp(clients[i].pass, cmd.str2))){
                dprint("login success : name(%s) pass(%s)\n",clients[i].name, clients[i].pass);
                send_cmd.func_num2 = SUCCESS;
                send_command(clients[i].socket,send_cmd);
            }else{
                continue;
            }
        }
#else
        int ret = 0;
        for(int i=0; i<nclient ; i++){
            if(!strcmp(clients[i].name, cmd.str1)){
                eprint("Exist User : name(%s)\n",clients[i].name);
                ret = -1;
            }
        }
        if(ret == -1){
            dprint("login fail : name(%s) \n",clients[cmd.client_id].name);
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "이미 존재하는 이름입니다.");
        }else{
            strcpy(clients[cmd.client_id].name, cmd.str1);
            clients[cmd.client_id].roomId = 0;

            strcpy(send_cmd.str1, "lobby");
            send_cmd.func_num2 = SUCCESS;
            dprint("login success : name(%s) \n",clients[cmd.client_id].name);
        }
        dprint("siganl to child(%d)\n",clients[cmd.client_id].pipe);
        send_command(clients[cmd.client_id].pipe, clients[cmd.client_id].pid,send_cmd);
        
        
#endif
        break;
    case CMD_ROOM_ADD:
        room_index = find_room_by_name(rooms, cmd.str1);
        dprint("room_index = %d\n",room_index);
        if(room_index < 0){
            room_index = find_empty_room_index(rooms);
            dprint("empty room_index = %d\n",room_index);
            if(room_index < 0){
                send_cmd.func_num2 = FAIL;
                strcpy(send_cmd.str1, "방 생성 실패!(자리 없음...).");
            }else{
                send_cmd.func_num2 = SUCCESS;
                strcpy(send_cmd.str1, "방을 생성합니다.");
                rooms[room_index].id = (++room_n);
                strncpy(rooms[room_index].name, cmd.str1, 99);
                strncpy(rooms[room_index].pass, cmd.str2, 99);
                rooms[room_index].type = 1;
            }
        }else{
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "방 생성 실패!(이미 존재하는 이름).");
        }
        send_command(clients[cmd.client_id].pipe, clients[cmd.client_id].pid, send_cmd);
        break;
    case CMD_ROOM_RM:
        /* code */
        break;
    case CMD_ROOM_JOIN:
        room_index = find_room_by_name(rooms, cmd.str1);
        if(room_index < 0){
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "잘못된 방 이름 입니다.");
            dprint("잘못된 방 이름 입니다.(%s)(%d)\n", cmd.str1, room_index);
        }else{
            if(!strcmp(cmd.str2, rooms[room_index].pass)){
                send_cmd.func_num2 = SUCCESS;
                clients[cmd.client_id].roomId = rooms[room_index].id;
                rooms[room_index].clients[rooms[room_index].nclient] = &clients[cmd.client_id];
                rooms[room_index].nclient++;
                strcpy(send_cmd.str1, "방에 입장합니다.");
            }else{
                send_cmd.func_num2 = FAIL;
                strcpy(send_cmd.str1, "잘못된 방 비밀번호 입니다.");
                dprint("잘못된 방 비밀번호 입니다.(%s)\n", cmd.str1);
            }
        }
        send_command(clients[cmd.client_id].pipe, clients[cmd.client_id].pid, send_cmd);
        break;
    case CMD_ROOM_PASS:
        /* code */
        break;
    case CMD_ROOM_LEAVE:
        if(clients[cmd.client_id].roomId==0){
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "방 나가기 실패 (로비입니다).");
        }else{
            room_index = find_room_by_id(rooms, clients[cmd.client_id].roomId);
            rooms[room_index].nclient--;
            clients[cmd.client_id].roomId = 0;
            send_cmd.func_num2 = SUCCESS;
            strcpy(send_cmd.str1, "방 나가기 성공 (로비입니다).");
        }
        send_command(clients[cmd.client_id].pipe, clients[cmd.client_id].pid, send_cmd);
        break;
    case CMD_ROOM_LIST:
        /* code */
        break;
    case CMD_ROOM_USERS:
        /* code */
        break;
    case CMD_SEND_ROOM:
        dprint("start broadcast!!! \n");
        broadcastChat(&(clients[cmd.client_id]), rooms, cmd);
        break;
    case CMD_SEND_WHISPER:
        /* code */
        break;
    default:
        break;
    }
}

// | client_id | func_num1 | func_num2 | input1 | input2 |
void signalUsr1Handler(){
    char command[BUFSIZ];
    int n;
    memset(command, 0, BUFSIZ);
    if((n=read(pfd[0],command,BUFSIZ))>0){
        func(command);
    }
}

void signalUsr2Handler(){
    char command[BUFSIZ];
    int n,size;
    memset(command, 0, BUFSIZ);
    if((n=read(cpfd[0], command, BUFSIZ))>0){
        dprint("sigusr2 : cmd(%s)\n",command);
        if(size = send(sockfd, command, BUFSIZ, MSG_DONTWAIT)<=0){
            perror("send()");
        }
    }
}