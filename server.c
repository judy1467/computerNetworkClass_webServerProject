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

void error_handling(char* message);
void GET_handler();
void generate_response();


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

    clnt_addr_size = sizeof(clnt_addr);

    // when client connect to server accept
    // accept() return to server client's descriptor number (maybe)
    // accept() blocks excute process
    if((clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size)) == -1) error_handling("accept error!\n");

    // after client's connection call handler
    GET_handler();


    // To fill it



    close(clnt_sock);
    close(serv_sock);

    return 0;
}

void error_handling(char* message){
    printf("%s", message);
    exit(1);
}

void GET_handler(){
    int file, length;

    if(recv(clnt_sock, recv_data, sizeof(recv_data)-1, 0) == -1) error_handling("recv error!]n");
    printf("%s\n", recv_data); // for check recv_data

    char* first_line[3];

    first_line[0] = strtok(recv_data, " "); // method
    first_line[1] = strtok(NULL, " "); // request file
    first_line[2] = strtok(NULL, " "); // http version

    printf("method: %s\n", first_line[0]);
    printf("file: %s\n", first_line[1]);
    printf("http version: %s\n", first_line[2]);

    if(strcmp(first_line[1], "/") == 0){
        send(clnt_sock, "HTTP/1.0 200 OK\n\n", 17, 0);
        char default_path[1024] = "";
        strcat(default_path, getenv("PWD"));
        strcat(default_path, "/default.html");

        printf("requested file is empty\n");
        file = open(default_path, O_RDONLY);
        while(1){
            length = read(file, send_data, BUFSIZE);
            printf("length: %d\n", length);
            if(length <= 0) break;
            write(clnt_sock, send_data, length);
        }
    }
    else{
        // concatenate path & filename
        char path[1024] = "";
        strcat(path, getenv("PWD"));

        // CLion에서 돌릴 때는 밑에꺼 주석 풀어줘야함, 터미널에서 돌릴 때는 주석 처리하고
        //    strcat(path, "/..");
        strcat(path, first_line[1]);
        printf("path: %s\n", path);

        // check the file
        int mode = R_OK | W_OK;
        if(access(path, mode) == 0){
            send(clnt_sock, "HTTP/1.0 200 OK\n\n", 17, 0);
            printf("File is exist\n");

            file = open(path, O_RDONLY);
            while(1){
                length = read(file, send_data, BUFSIZE);
                if(length <= 0) break;
                printf("length: %d\n", length);
                write(clnt_sock, send_data, length);
            }
            printf("[sending file done!!]\n");

        }
        else{
            printf("File is not exist\n");
            write(clnt_sock, "HTTP/1.1 404 Not Found\n", 23);
        }
    }

    // check the file
//	int mode = R_OK | W_OK;
//	if(access(path, mode) == 0){
//		printf("File is exist\n");
//        send(clnt_sock, "HTTP/1.0 200 OK\n", 16, 0);
//
//        file = open(path, O_RDONLY);
//        while(1){
//            length = read(file, send_data, BUFSIZE);
//            if(length <= 0) break;
//            printf("length: %d\n", length);
////            printf("length: %d\n", length);
////            send(clnt_sock, send_data, sizeof(send_data), 0);
//            write(clnt_sock, send_data, length);
//        }
//        printf("check\n");
//
//	}
//	else{
//		printf("File is not exist\n");
//        send(clnt_sock, "HTTP/1.1 404 Not Found\n", 23, 0);
//	}
}

void generate_response(){

}