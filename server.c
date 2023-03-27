#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

struct sockaddr_in serv_addr, clnt_addr;
socklen_t clnt_addr_size;
int serv_sock, clnt_sock;
char recv_data[BUFSIZE];

void error_handling(char* message);
void GET_handler(int* arg);


int main(int argc, char* argv[])
{
    if(argc != 2) error_handling("parameter error!\n");

    // Create server socket
    if((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) error_handling("socket error!\n");

    // initialize serv_addr & setting
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    // bind serv_sock with serv_addr's data
    if(bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) error_handling("bind error!\n");

    // wait client's connection, second parameter: waiting queue size
    if(listen(serv_sock, 5) == -1) error_handling("listen error!\n");

    clnt_addr_size = sizeof(clnt_addr);

    // when client connect to server accept
    if((clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size)) == -1) error_handling("accept error!\n");

    // after client's connection call handler
    GET_handler(&clnt_sock);


    close(clnt_sock);
    close(serv_sock);

    return 0;
}

void error_handling(char* message){
    printf("%s", message);
    exit(1);
}

void GET_handler(int* arg){
    int tmp;
    if((tmp = recv(clnt_sock, recv_data, sizeof(recv_data)-1, 0)) == -1) error_handling("recv error!]n");
    printf("%s\n\n", recv_data); // for check recv_data

    char* method;
    strtok(recv_data, " ");
    printf("method: %s\n", recv_data);

}