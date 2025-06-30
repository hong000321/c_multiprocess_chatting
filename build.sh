
echo "========================== build server ============================"
gcc -o chat_server.out chat_server.c child_chat_server.c parent_chat_server.c command.c chat_server_global.c 



echo "========================== build client ============================"
gcc -o chat_client.out chat_client.c command.c
