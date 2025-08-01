#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <libgen.h>
#include <fcntl.h>

#define CONNECTION_PORT 4557
#define CONNECTION_IP   "127.0.0.1"
#define FILE_PATH_LEN   100
#define BUFFER_LEN      1024

typedef enum{
    SUCCESS,
    FAILED
}STATUS;

STATUS Recv_Cmd(int sockfd, int *choice) {
    char cmd;
    int recv_bytes = read(sockfd, &cmd, sizeof(cmd));
    if (recv_bytes <= 0) {
        return FAILED;
    }
    *choice = (cmd == 'S') ? 2 : (cmd == 'R') ? 1 : 3;
    return SUCCESS;
}

STATUS Send_File(int sockfd) {
    int recv_bytes, send_bytes;
   
    //Receive metadata
    int fileNameLen;
    recv_bytes = read(sockfd, &fileNameLen, sizeof(fileNameLen));
    if(recv_bytes <=0) return FAILED;

    char filename[fileNameLen+1];
    recv_bytes = read(sockfd, filename, fileNameLen);
    if (recv_bytes != fileNameLen) return FAILED;
    filename[fileNameLen] = '\0';

    //open file
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        send_bytes = write(sockfd, "NOT FOUND", sizeof("NOT FOUND"));
        if (send_bytes <= 0) return FAILED;
        perror("[-] file opening Failed\n");
        return FAILED;
    }
    send_bytes = write(sockfd, "FOUND", sizeof("FOUND"));
    if (send_bytes <= 0) return FAILED;

    int fileSize;
    struct stat file_info;
    if (stat(filename, &file_info) == 0) {
        fileSize = file_info.st_size;
        send_bytes = write(sockfd, &fileSize, sizeof(fileSize));
        if (send_bytes <= 0) return FAILED;
    } else {
        perror("stat failed");
        return FAILED;
    }

    printf("Sending.....\nFile Path Len: %d\nFile Path: %s\nFile Size: %d\n", fileNameLen, filename, fileSize);   
    
    //Send file data
    int total_bytes = 0;
    char buffer[BUFFER_LEN];
    while(total_bytes < fileSize) {
        recv_bytes = read(fd, buffer, sizeof(BUFFER_LEN));
        if(recv_bytes <= 0) return FAILED;
        send_bytes = write(sockfd, buffer, recv_bytes);
        if (send_bytes != recv_bytes) return FAILED;
        total_bytes += recv_bytes;
    }

    //close file
    close(fd);
    return (total_bytes == fileSize) ? SUCCESS : FAILED;
}


STATUS Recv_File(int sockfd) {
    int recv_bytes, send_bytes;
    //Receive metadata
    printf("Receiving file\n");
    int fileNameLen;
    recv_bytes = read(sockfd, &fileNameLen, sizeof(fileNameLen));
    if (recv_bytes <= 0) return FAILED;

    char filename[fileNameLen+1];
    recv_bytes = read(sockfd, filename, fileNameLen);
    if (recv_bytes <= 0) return FAILED;
    filename[fileNameLen] = '\0';
    
    int fileSize;
    recv_bytes = read(sockfd, &fileSize, sizeof(fileSize));
    if (recv_bytes <= 0) return FAILED;
    printf("File Name Len: %d\nFile name: %s\nFile Size: %d\n", fileNameLen, filename, fileSize);
    
    //open file
    int fd = open(filename, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror("[-] file opening Failed\n");
        return FAILED;
    }
    //receive data
    char buffer[BUFFER_LEN];
    int total_bytes = 0;
    while(total_bytes < fileSize) {
        recv_bytes = read(sockfd, buffer, BUFFER_LEN);
        if (recv_bytes <= 0) return FAILED;
        send_bytes = write(fd, buffer, recv_bytes);
        if (send_bytes != recv_bytes) return FAILED;
        total_bytes += recv_bytes;
    }

    //close file
    close(fd);
    return (total_bytes == fileSize) ? SUCCESS : FAILED;
}

int main() {

    //Socket Creation
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-] Socket Creation Failed\n");
        exit(EXIT_FAILURE);
    }

    //Option setting to server
    int status;
    int optVal = 1;
    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    if (status < 0) {
        perror("[-] Option setting to Server Failed\n");
        exit(EXIT_FAILURE);
    }

    //Binding Address to Server
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CONNECTION_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_zero[8] = '\0';

    status = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (status < 0) {
        perror("[-] Binding to Server Failed\n");
        exit(EXIT_FAILURE);
    }

    //Listening to Socket
    status = listen(sockfd, 4);
    if (status < 0) {
        perror("[-] Listening Failed\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Listening .....\n");

    //Accepting connection
    int newSockfd;
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    newSockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &len);
    if (newSockfd < 0) {
        perror("[-] Connection to Client Failed\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Connected to client %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    close(sockfd);

    while(1) {
        int choice;
        
        if (Recv_Cmd(newSockfd, &choice) != SUCCESS) {
            printf("[-] Receving Command Failed\nRetry...\n");
            continue;
        }
        switch(choice) {
            case 1:
            {
                if (Send_File(newSockfd) == SUCCESS) printf("[+] File Sent Successfully\n");
                else printf("[-] Failed to send...Retry\n");
            }
            break;
            case 2:
            {
                if (Recv_File(newSockfd) == SUCCESS) printf("[+] File Received Successfully\n");
                else printf("[-] Failed to Receive...Retry\n");
            }
            break;
            case 3:
            close(newSockfd);
            exit(0);
        }
        
    }

    close(newSockfd);   
    return 0;

}