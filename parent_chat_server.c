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
    g_rooms[0].id = 0;
    strcpy(g_rooms->name, "lobby");
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
    dprint("start broadcast!!! cid(%d)\n",cmd.cid);
    struct client_s *from_client = &g_clients[find_client_index_by_id(cmd.cid)];
    struct command_s ret_command;
    char cmd_str[BUFSIZ];
    ret_command.cid = from_client->id;
    ret_command.func_num1 = cmd.func_num1;
    ret_command.func_num2 = SUCCESS;
    if(from_client->roomId==0){
        eprint("is lobby(%s)\n",from_client->name);
        sprintf(ret_command.str1, "현재 로비에 있습니다. 방으로 이동해 주세요.");
        send_command(from_client->pipe[1], from_client->pid, ret_command);
        return -1;
    }
    int room_index = find_room_by_id(g_rooms,from_client->roomId);
    dprint("from(%d) room_index(%d), nclients(%d), strBuf(%s)\n",from_client->id,room_index, g_rooms[room_index].nclients, cmd.str2);
    
    
    strcpy(ret_command.str1,from_client->name);
    strcpy(ret_command.str2,cmd.str1);
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


int cmd_login(const char *name, const char *pass, int cid){ // cid 는 접속한 client와 server의 child가 연결된 시점에 저장된 id
    dprint("CMD_ROBBY_LOGIN\n");
    int ret = -1;
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROBBY_LOGIN;
    memset(send_cmd.str1,0,100);
    memset(send_cmd.str2,0,BUFSIZ);
    ret = 0;
    
    for(int i=0; i<MAX_USER ; i++){
        if(g_clients[i].id < 0){ // 사용하지 않는 클라이언트
            continue;
        }
        if(!strcmp(g_clients[i].name, name) && !strcmp(g_clients[i].pass, pass)){ // 기존에 있는 ID/PASS와 비교해서 일치하면
            if(g_clients[i].id >= 0){
                iprint("login failed : 이미 접속중인 유저입니다.\n");
                send_cmd.func_num2 = FAIL;
                strcpy(send_cmd.str1, "이미 접속중인 유저입니다.");
                ret = -1;
                break;
            }
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
        }else if(!strcmp(g_clients[i].name, name)){ // ID만 일치하면
            iprint("login fail : name(%s) pass(%s)\n",g_clients[i].name, g_clients[i].pass);
            send_cmd.func_num2 = FAIL;
            strcpy(send_cmd.str1, "이미 존재하는 이름입니다.");
            ret = -1;
            break;
        }else{ // ID/PASS 둘 다 일치 하지 않으면 회원가입으로 간주
            ret = 1;
        }
    }
    if(ret == 1){  // 회원가입 : ID/PASS 둘 다 일치 하지 않으면 회원가입으로 간주
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
    dprint("CMD_ROOM_ADD\n");
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

            g_rooms[room_index].id = (g_room_num++);
            strncpy(g_rooms[room_index].name, room_name, 99);
            if(strcmp(room_pass,"")){
                iprint("비밀방을 생성합니다.\n   req-user: %s \n   req-room: %s\n", g_clients[client_index].name, room_name);
                strcpy(send_cmd.str1, "비밀방을 생성합니다.");
                strcpy(send_cmd.str2, room_name);
                strncpy(g_rooms[room_index].pass, room_pass, 99);
                g_rooms[room_index].type = 1; // 비밀방
            }else{
                iprint("공개방을 생성합니다.\n   req-user: %s \n   req-room: %s\n", g_clients[client_index].name, room_name);
                strcpy(send_cmd.str1, "공개방을 생성합니다.");
                strcpy(send_cmd.str2, room_name);
                strncpy(g_rooms[room_index].pass, room_pass, 99);
                g_rooms[room_index].type = 0; // 공개방
            }
            
            ret = 0;
        }
    }
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return ret;
}

int cmd_room_rm(const char *room_name, const char *room_pass, int cid){
    dprint("CMD_ROOM_RM\n");
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
        for(int i=0; i<MAX_USER; i++){
            if(g_rooms[room_index].clients[i]){
                g_rooms[room_index].clients[i]->roomId = -1; // 유저 강퇴
            }
        }
        g_rooms[room_index].id = -1; // 방 제거
        ret = 0;
    }
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return ret;
}

int cmd_room_join(const char *room_name, const char *room_pass, int cid){
    dprint("CMD_ROOM_JOIN\n");
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
        sprintf(send_cmd.str1, "방에 참가합니다[%s].",room_name);
        iprint("방에 참가합니다.\n   req-user: %s \n   req-room: %s\n", g_clients[client_index].name, room_name);
        g_rooms[room_index].clients[g_rooms[room_index].nclients++] = &g_clients[client_index];
        g_clients[client_index].roomId = g_rooms[room_index].id;
        ret = 0;
    }
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return ret;
}

int cmd_room_leave(int cid){
    dprint("CMD_ROOM_LEAVE\n");
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
    }if(room_index == 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "이미 로비입니다.");
        iprint("이미 로비입니다.\n");
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
    dprint("CMD_ROOM_LIST\n");
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_LIST;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    send_cmd.func_num2 = SUCCESS;
    strcpy(send_cmd.str1, "방 리스트입니다.");
    iprint("방 리스트 요청: %s(%d)\n", g_clients[client_index].name, cid);
    
    char room_list[BUFSIZ] = "";
    int room_cnt = 0;
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
        room_cnt++;
    }
    if(room_cnt==0){
        strcat(room_list,"방이 없습니다.. 방을 추가해주세요");
    }
    
    strncpy(send_cmd.str2, room_list, BUFSIZ-1);
    
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return 0;
}

int cmd_room_users(int cid){
    dprint("CMD_ROOM_USERS\n");
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_USERS;
    if(g_clients[client_index].roomId == 0){
        send_cmd.func_num2 = FAIL;
        strcpy(send_cmd.str1, "로비에서는 사용할 수 없는 명령입니다.");
        iprint("로비에서는 사용할 수 없는 명령입니다.\n");
    }else{
        char room_name[100];
        int room_index = find_room_by_id(g_rooms, g_clients[client_index].roomId);
        memset(room_name, 0, 100);
        strncpy(room_name, g_rooms[room_index].name, 100);
        
        memset(send_cmd.str1, 0, CMD_STR1_SIZE);
        memset(send_cmd.str2, 0, BUFSIZ);
        
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
    }
    
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return 0;
}

int cmd_send_whisper(const char *to_name, const char *message, int cid){
    dprint("CMD_WHISPER\n");
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_WHISPER;
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

int cmd_help(int cid){
    dprint("CMD_HELP\n");
    struct command_s send_cmd;
    int client_index = find_client_index_by_id(cid);
    send_cmd.func_num1 = CMD_ROOM_HELP;
    memset(send_cmd.str1, 0, CMD_STR1_SIZE);
    memset(send_cmd.str2, 0, BUFSIZ);
    
    send_cmd.func_num2 = SUCCESS;
    strcpy(send_cmd.str1, "커맨드 정보 : ");
    
    sprintf(send_cmd.str2, "%s",
                            "/add room1 0321             : 채팅방 room1을 비밀번호 0321로 생성\n"
                            "/add room2                  : 채팅방 room2를 비밀번호 없이 생성\n"
                            "/join room1 0321            : room1 채팅방에 비밀번호 0321로 입장\n"
                            "/join room2                 : room2 채팅방에 입장 (비밀번호 없음)\n"
                            "/leave                      : 현재 채팅방에서 퇴장\n"
                            "/list                       : 생성된 채팅방 목록 출력\n"
                            "/users                      : 현재 채팅방에 있는 유저 목록 출력\n"
                            "/rm room1 0321              : 비밀번호 0321로 room1 비밀 채팅방 삭제\n"
                            "/rm room2                   : 비밀번호 없이 room2 공개 채팅방 삭제\n"
                            "[일반메시지]                  : 접속 중인 채팅방에 메시지 전송\n"
                            "!whisper 홍대오 안녕하세요~~~  : 홍대오에게 귓속말 전송\n"
                            "/exit                       : 채팅 프로그램 종료\n"
                            "/help                       : 사용 가능한 명령어 목록 출력\n"
                        );
    send_command(g_clients[client_index].pipe[1], g_clients[client_index].pid, send_cmd);
    return 0;

}

int cmd_exit(int cid){
    dprint("CMD_EXIT\n");
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
        ret = cmd_login(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_ADD:      // 로비에서 방 추가 커맨드
        ret = cmd_room_add(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_RM:       // 로비에서 방 제거 커맨드
        ret = cmd_room_rm(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_JOIN:     // 로비에서 방 참가 커맨드
        ret = cmd_room_join(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_LEAVE:    // 방에서 로비로 나가기 커맨드
        ret = cmd_room_leave(cmd.cid);
        break;
    case CMD_ROOM_LIST:     // 로비에서 방 리스트 확인 커맨드
        ret = cmd_room_list(cmd.cid);
        break;
    case CMD_ROOM_USERS:    // 방에서 유저 수 확인 커맨드
        ret = cmd_room_users(cmd.cid);
        break;
    case CMD_ROOM_SEND:     // 채팅 브로드캐스팅 커맨드
        broadcastChat(cmd);
        break;
    case CMD_WHISPER:       // 채팅 귓속말 커맨드
        ret = cmd_send_whisper(cmd.str1, cmd.str2, cmd.cid);
        break;
    case CMD_ROOM_HELP:
        ret = cmd_help(cmd.cid);
        break;
    case CMD_EXIT:          // 클라이언트 종료 커맨드
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