#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdlib.h>
//#define DEBUG (4*1 + 3)
#include "chat_server.h"
#include "utils.h"
#include "chat_server_global.h"
#include "child_chat_server.h"
#include "parent_chat_server.h"

int main(int argc, char **argv){
    int ret = 0;
    int mainSocket, clientSocket;
    pid_t broadcastPid, clientPid;
    int ppfd[2];
    int cpfd[2];
    int ppid = getpid();
    struct sockaddr_in cliaddr;
    socklen_t clen = sizeof(struct sockaddr_in);

    ret = createServerSocket(argc, argv);
    if(ret < 0){
        eprint("server 소켓 생성 실패\n");
        return -1;
    }else{
        mainSocket = ret;
    }

    init_parent();
    
    
    // 파이프 생성
    
    
    signal(SIGINT,sigstop_handler);
    signal(SIGTERM,sigstop_handler);
    // signal(SIGUSR1,sigusr1_handler);
    struct sigaction sa;
    sa.sa_sigaction = sigusr1_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);


    
    do{
        clientSocket = accept(mainSocket, (struct sockaddr*)&cliaddr, &clen);
        if(clientSocket > 0){
            if(pipe(cpfd)){
                perror("to child pipe()");
                break;
            }
            if(pipe(ppfd)){
                perror("to parent pipe()");
                return -1;
            }
            if((clientPid=fork())==0){
                if(run_child_process(clientSocket, ppid, nclient, ppfd[1], cpfd[0]) == -1){
                    goto exit;
                }
            }else{
                run_parent_work(clientPid, nclient, ppfd[0], cpfd[1]);
                nclient++;
            }
        }
    }while(!stop);

exit:
    return 0;
}

// ====================================================================
// ========================= member functions =========================
// ====================================================================
int createServerSocket(int argc, char **argv){
    int mainSocket;
    struct sockaddr_in servaddr;
    
    // 주소와 포트 설정
    int port = 5101;
    char *address = "127.0.0.1"; //"192.168.2.95";
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

    memset(&servaddr, 0 , sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port        = htons(port);
    iprint("Server Net = %s:%d\n",address,port);
    
    if(bind(mainSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("bind()");
        return -1;
    }

    if(listen(mainSocket, 8)<0){
        perror("listen");
        return -1;
    }
    return mainSocket;
}



void sigstop_handler(int signo){
    stop = 1;
}

