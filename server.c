#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

#define BUFSIZE 1024

struct sockaddr_in serv_addr, clnt_addr;
socklen_t clnt_addr_size;
int serv_sock, clnt_sock;
char recv_data[BUFSIZE];
char send_data[BUFSIZE];
char* method;
char* request_file;
char* http_version;

void error_handling(char* message);
void GET_handler();
void request_handler();
void generate_header();

int main(int argc, char* argv[])
{
//    if(argc != 2) error_handling("parameter error!\n");

    // Create server socket
    // socket()의 리턴 값은 고유 디스크립터 번호임, 에러는 -1반환
    if((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) error_handling("socket error!\n");

    // initialize serv_addr & setting
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
//    serv_addr.sin_port=htons(atoi(argv[1]));
    char port[] = "1467";
    serv_addr.sin_port=htons(atoi(port));

    // bind serv_sock with serv_addr's data
    // socket()을 통해 반환받은 디스크립터 번호를 첫 번째 인자로 받음.
    // Client의 request가 있기 전 까지 리턴하지 않음. (wait client's request)
    if(bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) error_handling("bind error!\n");

    // wait client's connection, second parameter: waiting queue size
    if(listen(serv_sock, 5) == -1) error_handling("listen error!\n");

    while(1) {
        clnt_addr_size = sizeof(clnt_addr);
        // when client connect to server accept
        // accept() return to server client's descriptor number (maybe)
        // accept() blocks excute process
        if ((clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size)) == -1){
            error_handling("accept error!\n");
            break;
        }
        // after client's connection call handler
//        GET_handler();
        request_handler();
    }


    close(clnt_sock);
    close(serv_sock);

    return 0;
}

void error_handling(char* message){
    printf("%s", message);
    exit(1);
}

void request_handler(){
    if(recv(clnt_sock, recv_data, sizeof(recv_data)-1, 0) == -1) error_handling("recv error!]n");
    printf("%s\n", recv_data); // for check recv_data

    char* first_line[3];

    method = (first_line[0] = strtok(recv_data, " ")); // method
    request_file = (first_line[1] = strtok(NULL, " ")); // request file
    http_version = (first_line[2] = strtok(NULL, " ")); // http version

    printf("method: %s\n", method);
    printf("file: %s\n", request_file);
    printf("protocol version: %s\n", http_version);

    if(strncmp(method, "GET", 3) == 0) GET_handler();

    printf("[close socket]\n--------------------------\n");
    close(clnt_sock);

}

void GET_handler(){
    int file, length;
    memset(send_data, 0, sizeof(send_data));

    if(strcmp(request_file, "/") == 0){
        printf("[requested file is empty]\n");

        // send status code(200)
        send(clnt_sock, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);

        char default_path[1024] = "";
        strcat(default_path, getenv("PWD"));
        strcat(default_path, "/..");
        strcat(default_path, "/default.html");

        file = open(default_path, O_RDONLY);
        while(1){
            length = read(file, send_data, BUFSIZE);
            if(length <= 0) break;
            write(clnt_sock, send_data, length);
        }
    }
    else{
        // concatenate path & filename
        char path[1024] = "";
        strcat(path, getenv("PWD"));

        // CLion에서 돌릴 때는 밑에꺼 주석 풀어줘야함, 터미널에서 돌릴 때는 주석 처리하고
        strcat(path, "/..");
        strcat(path, request_file);
        printf("path: %s\n", path);

        // check the file
        int mode = R_OK | W_OK;
        if(access(path, mode) == 0){

            printf("[File is exist!]\n");

            // open, read는 c라이브러리의 fopen, fread와는 달리 리눅스에서 제공하는 함수
            if((file = open(path, O_RDONLY)) != -1){
                // 타입마다 헤더를 따로 붙여줘야 함 아니면 웹브라우저에서 제대로 표시를 못 함
                generate_header();
                send(clnt_sock, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
                int total_size = 0;
                while(1){
                    length = read(file, send_data, BUFSIZE);
                    total_size += length;
                    if(length == 0) break;
                    write(clnt_sock, send_data, length);
                }
                printf("file size = %dBytes\n[sending file done!!]\n", total_size);
            }

        }
        else{
            printf("[File is not exist]\n");
            write(clnt_sock, "HTTP/1.1 404 Not Found\n", 23);
        }
    }
}
