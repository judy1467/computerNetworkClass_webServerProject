#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 1024

struct sockaddr_in serv_addr, clnt_addr;
socklen_t clnt_addr_size;
int serv_sock, clnt_sock;
char recv_data[BUFSIZE];
char* method;
char* request_file;
char* http_version;

void error_handling(char* message);
void GET_handler();
void request_handler();
void generate_header(const unsigned long* file_size, char* path);
void read_request_message();
void send_file(char* path, int* file);
void check_file_existence(char* path, int* file, unsigned long* file_size);
void get_file_size(char* path, int* file, unsigned long* file_size);
char* file_type();

int main(int argc, char* argv[])
{
    if(argc != 2){
        printf("Usage: %s [port#]\n", argv[0]);
        error_handling("parameter error!\n");
    }

    // Create server socket
    // socket()의 리턴 값은 고유 디스크립터 번호임, 에러는 -1반환
    if((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) error_handling("socket error!\n");

    // initialize serv_addr & setting
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));
//    char port[] = "1467";
//    serv_addr.sin_port=htons(atoi(port));

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
    if(recv(clnt_sock, recv_data, sizeof(recv_data)-1, 0) == -1) error_handling("[recv error!]\n");

    // Part A:dump request message in console
    printf("%s\n", recv_data);

    read_request_message();

    if(request_file != NULL)
        if(strncmp(method, "GET", 3) == 0) GET_handler();

//    printf("[close socket]\n--------------------------\n");
    close(clnt_sock);
}

void GET_handler(){
    int file = 0;
    char path[1024];
    memset(path, 0, sizeof(path));
    unsigned long file_size = 0;

    if(strcmp(request_file, "/") == 0){

        // send status code(200)
        send(clnt_sock, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);

        // pwd + default page file(default.html)
        strcat(path, getenv("PWD"));
//        strcat(path, "/.."); // when run in CLion set it, but run in terminal unset it.
        strcat(path, "/default.html");

        send_file(path, &file);
    }
    else{
        // concatenate path & filename
        strcat(path, getenv("PWD"));

//        strcat(path, "/.."); // when run in CLion set it, but run in terminal unset it.
        strcat(path, request_file);

        check_file_existence(path, &file, &file_size);
    }
}

void generate_header(const unsigned long* file_size, char* path){
    char header[BUFSIZE];
    memset(header, 0, sizeof(header));

    // to get file's last-modified time
    struct stat st1;
    struct tm *tm1;
    char buf_tmp[64];

    if(lstat(path, &st1) == -1) error_handling("stat error!\n");
    tm1 = gmtime((const time_t *) &st1.st_mtimespec);
    strftime(buf_tmp, sizeof(buf_tmp), "%a, %d %b %Y %H:%M:%S GMT\r\n", tm1);

    // file 타입마다 헤더를 Content-type을 바꿔줘야함
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nAccept-Ranges: bytes\r\nServer: http\r\nLast-Modified: %s", file_type(), (*file_size), buf_tmp);

    // get date, time to attach at header
    time_t t;
    struct tm *tm2;
    char buf_time[64];
    memset(buf_time, 0, sizeof(buf_time));
    t = time(NULL);
    tm2 = gmtime(&t);
    strftime(buf_time, sizeof(buf_time), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n\r\n", tm2);
    strcat(header, buf_time);

    send(clnt_sock, header, strlen(header), 0);
}

void read_request_message(){
    char* first_line[3];

    method = (first_line[0] = strtok(recv_data, " ")); // method
    request_file = (first_line[1] = strtok(NULL, " ")); // request file
    http_version = (first_line[2] = strtok(NULL, " ")); // http version

//    printf("method: %s\n", method);
//    printf("file: %s\n", request_file);
//    printf("protocol version: %s\n", http_version);
}

void send_file(char* path, int* file){
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    size_t bytes_read;
    FILE *fp = NULL;
    if((fp = fopen(path, "rb")) == NULL) error_handling("file open error!\n");

    while((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(clnt_sock, buffer, bytes_read, 0);

        // when send .mp3 file occur "Terminated due to signal 13" so, add this code
        signal(SIGPIPE, SIG_IGN);
    }

//    printf("[send done]\n");
    fclose(fp);
}

void check_file_existence(char* path, int* file, unsigned long* file_size){
    // check the file
    int mode = R_OK | W_OK;
    if(access(path, mode) == 0){
//        printf("[File exist]\n");

        get_file_size(path, file ,file_size);
        generate_header(file_size, path);
        send_file(path, file);

//        printf("file size: %lu\n", *file_size);
    }
    else{
//        printf("[File not exist]\n");
        send(clnt_sock, "HTTP/1.1 404 Not Found\r\n", 24, 0);
    }
}

void get_file_size(char* path, int* file, unsigned long* file_size){
    (*file) = open(path, O_RDONLY);
    int length = 0, tmp[BUFSIZE];
    memset(tmp, 0, BUFSIZE);

    // read file data and send to clnt_sock
    while(1){
        length = read((*file), tmp, BUFSIZE);
        if(length <= 0) break;

        (*file_size) += length;
        length = 0;
        memset(tmp, 0, sizeof(tmp));
    }
    close(*file);
}

char* file_type(){
    if(strstr(request_file, ".html") !=  NULL)
        return "text/html";
    else if(strstr(request_file, ".htm") !=  NULL)
        return "text/html";
    else if(strstr(request_file, ".css") !=  NULL)
        return "text/css";
    else if(strstr(request_file, ".xml") !=  NULL)
        return "text/xml";
    else if(strstr(request_file, ".png") != NULL)
        return "image/png";
    else if(strstr(request_file, ".jpg") != NULL)
        return "image/jpeg";
    else if(strstr(request_file, ".mp3") != NULL)
        return "audio/mpeg";
    else if(strstr(request_file, ".pdf") != NULL)
        return "application/pdf";
    else if(strstr(request_file, ".doc") != NULL)
        return "application/msword";
    else if(strstr(request_file, ".js") != NULL)
        return "application/x-javascript";
    else if(strstr(request_file, ".zip") != NULL)
        return "application/zip";
    else if(strstr(request_file, ".mov") != NULL)
        return "video/quicktime";
    else if(strstr(request_file, ".mp4") != NULL)
        return "video/mp4";
    else if(strstr(request_file, ".mpeg") != NULL || strstr(request_file, ".mpg") != NULL || strstr(request_file, ".m4v") != NULL)
        return "video/mpeg";
    else if(strstr(request_file, ".avi") != NULL)
        return "video/x-msvideo";
    else
        return "text/plain";
}