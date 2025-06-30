#include "parent_chat_server.h"
#include "command.h"
#include <signal.h>


struct client_s g_clients[MAX_USER];
struct room_s g_rooms[MAX_ROOM];
int g_room_num=0;

void init_parent(){
    // 방 초기화
    for(int i=0; i<MAX_ROOM; i++){
        g_rooms[i].id = -1;
    }

    // 클라이언트 초기화
    for(int i=0; i<MAX_ROOM; i++){
        g_clients[i].id = -1;
    }
    g_room_num++;
}

int find_room_by_id(struct room_s g_rooms[MAX_ROOM], int id){
    for(int i=0; i<MAX_ROOM ; i++){
        if(id == g_rooms[i].id){
            return i;
        }
    }
    return -1;
}

int find_room_by_name(struct room_s g_rooms[MAX_ROOM], const char room_name[100]){
    for(int i=0; i<MAX_ROOM ; i++){
        if(!strcmp(g_rooms[i].name, room_name)){
            return i;
        }
    }
    return -1;
}

int find_empty_room_index(){
    for(int i=0; i<MAX_ROOM; i++){
        if(g_rooms[i].id==-1){
            return i;
        }
    }
    return -1;
}

int find_empty_client_index(){
    for(int i=0; i<MAX_USER; i++){
        if(g_clients[i].id==-1){
            return i;
        }
    }
    return -1;
}

int find_client_index_by_id(int id){
    for(int i=0; i<MAX_USER; i++){
        if(g_clients[i].id==-1){
            continue;
        }
        if(g_clients[i].id==id){
            return i;
        }
    }
    return -1;
}

int find_client_index_by_pid(int pid){
    for(int i=0; i<MAX_USER; i++){
        if(g_clients[i].id==-1){
            continue;
        }
        if(g_clients[i].pid==pid){
            return i;
        }
    }
    return -1;
}

int find_client_index_by_name(const char *name){
    for(int i=0; i<MAX_USER; i++){
        if(g_clients[i].id==-1){
            continue;
        }
        if(strcmp(g_clients[i].name,name) == 0){
            return i;
        }
    }
    return -1;
}

int send_command(int fd, int pid, struct command_s cmd){
    char cmd_str[BUFSIZ];
    memset(cmd_str, 0, BUFSIZ);
    pack_command(cmd,cmd_str,BUFSIZ);
    vprint("cid(%d), fn1(%d), fn2(%d), str1(%s), str2(%s)\n", cmd.cid, cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
    
    write(fd,cmd_str,BUFSIZ);
    kill(pid, SIGUSR2);
    return 0;
}

int broadcastChat(struct command_s cmd){
    struct client_s *from_client = &g_clients[find_client_index_by_id(cmd.cid)];
    if(from_client->roomId==0){
        eprint("is lobby(%s)\n",from_client->name);
        return -1;
    }
    int room_index = find_room_by_id(g_rooms,from_client->roomId);
    dprint("from(%d) room_index(%d), nclients(%d), strBuf(%s)\n",from_client->id,room_index, g_rooms[room_index].nclients, cmd.str2);
    struct command_s ret_command;
    char cmd_str[BUFSIZ];
    ret_command.cid = from_client->id;
    ret_command.func_num1 = cmd.func_num1;
    ret_command.func_num2 = SUCCESS;
    strcpy(ret_command.str1,from_client->name);
    strcpy(ret_command.str2,cmd.str2);
    pack_command(ret_command,cmd_str,BUFSIZ);


    for(int i=0; i<g_rooms[room_index].nclients; i++){
        if(g_rooms[room_index].clients[i]->id == from_client->id){
            continue;
        }
        write(g_rooms[room_index].clients[i]->pipe[1],cmd_str,BUFSIZ);
        kill(g_rooms[room_index].clients[i]->pid, SIGUSR2);
    }
    return 0;
}


int cmd_login(const char *name, const char *pass, int cid){
    int ret = -1;
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROBBY_LOGIN;
    memset(send_cmd.str1,0,100);
    memset(send_cmd.str2,0,BUFSIZ);
    ret = 0;
    
    for(int i=0; i<MAX_USER ; i++){
        if(g_clients[i].id < 0){
            continue;
        }
        if(!strcmp(g_clients[i].name, name) && !strcmp(g_clients[i].pass, pass)){
            g_clients[i].id = cid;
            g_clients[i].pid = g_clients[client_index].pid;
            g_clients[i].pipe[0] = g_clients[client_index].pipe[0];
            g_clients[i].pipe[1] = g_clients[client_index].pipe[1];
            g_clients[i].roomId = 0; // 로비에 접속
            g_clients[client_index].id = -1; // 기존 클라이언트 제거
            g_clients[client_index].pid = -1;
            g_clients[client_index].pipe[0] = -1;
            g_clients[client_index].pipe[1] = -1;
            g_clients[client_index].roomId = -1;
            iprint("login success : name(%s) pass(%s)\n",g_clients[i].name, g_clients[i].pass);
            strcpy(send_cmd.str1, "로비입니다.");
            send_cmd.func_num2 = SUCCESS;
            ret = 0;
            break;
        }else if(!strcmp(g_clients[i].name, name)){
            iprint("login fail : name(%s) pass(%s)\n",g_clients[i].name, g_clients[i].pass);
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "이미 존재하는 이름입니다.");
            ret = -1;
            break;
        }else{
            ret = 1;
        }
    }
    if(ret == 1){
        send_cmd.func_num2 = SUCCESS;
        strncpy(g_clients[client_index].name, name, MAX_NAME-1);
        strncpy(g_clients[client_index].pass, pass, MAX_NAME-1);
        g_clients[client_index].roomId = 0;
        strcpy(send_cmd.str1, "로비입니다.");
        iprint("login success : name(%s)(%d)(%d) \n",g_clients[client_index].name, cid, client_index);
    }
    dprint("siganl to child(%d)\n",g_clients[client_index].pipe[1]);
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid,send_cmd);
    return 0;
}

int cmd_room_add(const char *room_name, const char *room_pass, int cid){
    int ret = 0;
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_ADD;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    int room_index = find_room_by_name(g_rooms, room_name);
    if(room_index >= 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "이미 존재하는 방 이름입니다.");
        iprint("이미 존재하는 방 이름입니다.(%s)\n", room_name);
        ret = -1;
    }else{
        room_index = find_empty_room_index();
        if(room_index < 0){
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "방 생성 실패!(자리 없음...).");
            iprint("방 생성 실패!(자리 없음...).\n");
            ret = -1;
        }else{
            send_cmd.func_num2 = SUCCESS;
            strcpy(send_cmd.str1, "방을 생성합니다.");
            iprint("방을 생성합니다.\n   req-user: %s \n   req-room: %s\n", g_clients[client_index].name, room_name);
            g_rooms[room_index].id = (g_room_num++);
            strncpy(g_rooms[room_index].name, room_name, 99);
            strncpy(g_rooms[room_index].pass, room_pass, 99);
            g_rooms[room_index].type = 1; // 비밀방
            ret = 0;
        }
    }
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return ret;
}

int cmd_room_rm(const char *room_name, const char *room_pass, int cid){
    int ret = 0;
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_RM;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    int room_index = find_room_by_name(g_rooms, room_name);
    if(room_index < 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "존재하지 않는 방 이름입니다.");
        iprint("존재하지 않는 방 이름입니다.(%s)\n", room_name);
        ret = -1;
    }else{
        if(strcmp(g_rooms[room_index].pass, room_pass) != 0){
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "방 비밀번호가 틀립니다.");
            iprint("방 비밀번호가 틀립니다.(%s)\n", room_name);
            ret = -1;
            return ret;
        }
        send_cmd.func_num2 = SUCCESS;
        strcpy(send_cmd.str1, "방을 제거합니다.");
        iprint("방을 제거합니다.\n   req-user: %s \n   req-room: %s\n", g_clients[client_index].name, room_name);
        g_rooms[room_index].id = -1; // 방 제거
        ret = 0;
    }
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return ret;
}

int cmd_room_join(const char *room_name, const char *room_pass, int cid){
    int ret = 0;
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_JOIN;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    int room_index = find_room_by_name(g_rooms, room_name);
    if(room_index < 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "존재하지 않는 방 이름입니다.");
        iprint("존재하지 않는 방 이름입니다.(%s)\n", room_name);
        ret = -1;
    }else if(strcmp(g_rooms[room_index].pass, room_pass) != 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "방 비밀번호가 틀립니다.");
        iprint("방 비밀번호가 틀립니다.(%s)\n", room_name);
        ret = -1;
    }else{
        send_cmd.func_num2 = SUCCESS;
        strcpy(send_cmd.str1, "방에 참가합니다.");
        iprint("방에 참가합니다.\n   req-user: %s \n   req-room: %s\n", g_clients[client_index].name, room_name);
        g_rooms[room_index].clients[g_rooms[room_index].nclients++] = &g_clients[client_index];
        g_clients[client_index].roomId = g_rooms[room_index].id;
        ret = 0;
    }
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return ret;
}

int cmd_room_leave(int cid){
    int ret = 0;
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    int room_index = find_room_by_id(g_rooms, g_clients[client_index].roomId);
    send_cmd.func_num1 = CMD_ROOM_LEAVE;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    if(room_index < 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "존재하지 않는 방 이름입니다.");
        iprint("존재하지 않는 방 이름입니다.(%s)\n", g_rooms[room_index].name);
        ret = -1;
    }else{
        send_cmd.func_num2 = SUCCESS;
        strcpy(send_cmd.str1, "방에서 나갑니다.");
        iprint("방에서 나갑니다.\n   req-user: %s \n   req-room: %s\n", g_clients[client_index].name, g_rooms[room_index].name);
        g_rooms[room_index].clients[g_rooms[room_index].nclients--] = NULL; // 방에서 나가기
        g_clients[client_index].roomId = 0; // 로비로 이동
        ret = 0;
    }
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return ret;
}

int cmd_room_list(int cid){
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_LIST;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    send_cmd.func_num2 = SUCCESS;
    strcpy(send_cmd.str1, "방 리스트입니다.");
    iprint("방 리스트 요청: %s(%d)\n", g_clients[client_index].name, cid);
    
    char room_list[BUFSIZ] = "";
    for(int i=0; i<MAX_ROOM; i++){
        if(g_rooms[i].id < 0){
            continue;
        }
        char room_info[100];
        if(g_rooms[i].type == 1){
            snprintf(room_info, sizeof(room_info), "Room ID: %d, Name: %s (비밀방)\n", g_rooms[i].id, g_rooms[i].name);
            iprint("방 정보: ID(%d), Name(%s), Type(비밀방)\n", g_rooms[i].id, g_rooms[i].name);
        }else{
            snprintf(room_info, sizeof(room_info), "Room ID: %d, Name: %s (공개방)\n", g_rooms[i].id, g_rooms[i].name);
            iprint("방 정보: ID(%d), Name(%s), Type(공개방)\n", g_rooms[i].id, g_rooms[i].name);
        }
        strcat(room_list, room_info);
    }
    
    strncpy(send_cmd.str2, room_list, BUFSIZ-1);
    
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return 0;
}

int cmd_room_users(const char *room_name, int cid){
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_USERS;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    int room_index = find_room_by_name(g_rooms, room_name);
    if(room_index < 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "존재하지 않는 방 이름입니다.");
        iprint("존재하지 않는 방 이름입니다.(%s)\n", room_name);
        return -1;
    }
    
    send_cmd.func_num2 = SUCCESS;
    strcpy(send_cmd.str1, "방 유저 리스트입니다.");
    
    char user_list[BUFSIZ] = "";
    for(int i=0; i<g_rooms[room_index].nclients; i++){
        char user_info[100];
        snprintf(user_info, sizeof(user_info), "User ID: %d, Name: %s\n", g_rooms[room_index].clients[i]->id, g_rooms[room_index].clients[i]->name);
        strcat(user_list, user_info);
    }
    
    strncpy(send_cmd.str2, user_list, BUFSIZ-1);
    
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return 0;
}

int cmd_send_whisper(const char *to_name, const char *message, int cid){
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_SEND_WHISPER;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    int to_index = find_client_index_by_name(to_name);
    if(to_index < 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "존재하지 않는 사용자입니다.");
        iprint("존재하지 않는 사용자입니다.(%s)\n", to_name);
        return -1;
    }
    
    send_cmd.func_num2 = SUCCESS;
    send_cmd.cid = cid;
    strcpy(send_cmd.str1, "귓속말입니다.");
    snprintf(send_cmd.str2, BUFSIZ-1, "[%s님의 귓속말]: %s", g_clients[client_index].name, message);
    
    send_command(g_clients[to_index].pipe[1], g_clients[to_index].pid, send_cmd);
    return 0;
}

int cmd_exit(int cid){
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_EXIT;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    send_cmd.func_num2 = SUCCESS;
    strcpy(send_cmd.str1, "클라이언트가 종료됩니다.");
    iprint("클라이언트 %s(%d) 종료됩니다.\n", g_clients[client_index].name, cid);
    
    // 클라이언트의 방에서 나가기
    if(g_clients[client_index].roomId > 0){
        cmd_room_leave(cid);
    }
    
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    
    // 클라이언트 정보 초기화
    g_clients[client_index].id = -2; // -2는 종료된 클라이언트
    g_clients[client_index].roomId = 0;
    return 0;
}

// | cid | func_num1 | func_num2 | input1 | input2 |
int command_logic(char *command){
    struct command_s cmd;
    memset(cmd.str1, 0, CMD_STR1_SIZE);
    memset(cmd.str2, 0, BUFSIZ);
    
    int room_index;
    int ret = unpack_command(&cmd,command);
    dprint("cid(%d), fn1(%d), fn2(%d), str1(%s), str2(%s)\n", cmd.cid, cmd.func_num1, cmd.func_num2, cmd.str1, cmd.str2);
    if(ret < 0){
        return -1;
    }
    switch (cmd.func_num1)
    {
    case CMD_ROBBY_LOGIN:       // 로비에서 로그인 커맨드
        dprint("CMD_ROBBY_LOGIN\n");
        ret = cmd_login(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_ADD:      // 로비에서 방 추가 커맨드
        dprint("CMD_ROOM_ADD\n");
        ret = cmd_room_add(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_RM:       // 로비에서 방 제거 커맨드
        dprint("CMD_ROOM_RM\n");
        ret = cmd_room_rm(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_JOIN:     // 로비에서 방 참가 커맨드
        dprint("CMD_ROOM_JOIN\n");
        ret = cmd_room_join(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_PASS:     // ????
        /* code */
        break;
    case CMD_ROOM_LEAVE:    // 방에서 로비로 나가기 커맨드
        dprint("CMD_ROOM_LEAVE\n");
        ret = cmd_room_leave(cmd.cid);
        break;
    case CMD_ROOM_LIST:     // 로비에서 방 리스트 확인 커맨드
        dprint("CMD_ROOM_LIST\n");
        ret = cmd_room_list(cmd.cid);
        break;
    case CMD_ROOM_USERS:    // 방에서 유저 수 확인 커맨드
        dprint("CMD_ROOM_USERS\n");
        ret = cmd_room_users(cmd.str1, cmd.cid);
        break;
    case CMD_SEND_ROOM:     // 채팅 브로드캐스팅 커맨드
        dprint("start broadcast!!! cid(%d)\n",cmd.cid);
        int client_index = find_client_index_by_id(cmd.cid);
        broadcastChat(cmd);
        break;
    case CMD_SEND_WHISPER:  // 채팅 귓속말 커맨드
        dprint("CMD_SEND_WHISPER\n");
        ret = cmd_send_whisper(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_EXIT:          // 클라이언트 종료 커맨드
        dprint("CMD_EXIT\n");
        ret = cmd_exit(cmd.cid);
        break;
    default:                // 에러
        eprint("unkown command!!!! \n");
        break;
    }
    printf("\n\n");
}

int run_parent_work(int clientPid, int client_id, int pread_fd, int cwrite_fd){
    int index = find_empty_client_index();
    if(index < 0){
        return -1;
    }
    g_clients[index].id = client_id;
    g_clients[index].pipe[0] = pread_fd;
    g_clients[index].pipe[1] = cwrite_fd;
    g_clients[index].roomId = 0;
    g_clients[index].pid = clientPid;
    return 0;
}


// | cid | func_num1 | func_num2 | input1 | input2 |
void sigusr1_handler(int signo, siginfo_t *info, void *context){
    char command[BUFSIZ];
    int n;
    int sender_pid = info->si_pid;
    int sender_index = find_client_index_by_pid(sender_pid);
    dprint("pid(%d), index(%d)\n",sender_pid,sender_index);
    for(int i=0; i<MAX_USER; i++){
        if(g_clients[i].id<0){
            continue;
        }
        memset(command, 0, BUFSIZ);
        if((n=read(g_clients[sender_index].pipe[0],command,BUFSIZ))>0){
            command_logic(command);
            break;
        }
    }
}