#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>

#define CONNECTION_PORT 4557
#define CONNECTION_IP   "127.0.0.1"
#define FILE_PATH_LEN   100
#define BUFFER_LEN      1024

typedef enum{
    SUCCESS,
    FAILED
}STATUS;

STATUS Send_Cmd(int sockfd, int choice) {
    char cmd = (choice == 1) ? 'S' : (choice == 2) ? 'R' : 'E';
    int send_bytes = write(sockfd, &cmd, sizeof(cmd));
    return ((send_bytes > 0) ? SUCCESS : FAILED);
}

STATUS Send_File(int sockfd) {
    int recv_bytes, send_bytes;
    //Read path from  user
    char file[FILE_PATH_LEN];
    printf("Enter file path: ");
    scanf("%s", file);

    //Send file metadata
    char *filename = basename(file);
    int fileNameLen = strlen(filename);
    send_bytes = write(sockfd, &fileNameLen, sizeof(fileNameLen));
    if (send_bytes <= 0) return FAILED;
    
    send_bytes = write(sockfd, filename, fileNameLen);
    if (send_bytes <= 0) return FAILED;

    int fileSize;
    struct stat file_info;
    if (stat(file, &file_info) == 0) {
        fileSize = file_info.st_size;
        send_bytes = write(sockfd, &fileSize, sizeof(fileSize));
        if (send_bytes <= 0) return FAILED;
    } else {
        perror("stat failed");
        return FAILED;
    }

    //open file
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("[-] file openinf Failed\n");
        return FAILED;
    }
    //Send file data
    int total_bytes = 0;
    char buffer[BUFFER_LEN];
    while(total_bytes < fileSize) {
        recv_bytes = read(fd, buffer, BUFFER_LEN);
        if (recv_bytes <= 0) return FAILED;
        send_bytes = write(sockfd, buffer, recv_bytes);
        if (send_bytes != recv_bytes) return FAILED;
        total_bytes += recv_bytes;
    }

    //close file
    close(fd);
    return SUCCESS;
}

STATUS Recv_File(int sockfd) {
    int recv_bytes, send_bytes;

    char file[FILE_PATH_LEN];
    printf("Enter file path: ");
    scanf("%s", file);

    //Send file metadata
    int fileNameLen = strlen(file);
    send_bytes = write(sockfd, &fileNameLen, sizeof(fileNameLen));
    if (send_bytes <= 0) return FAILED;
    send_bytes =write(sockfd, file, fileNameLen);
    if (send_bytes <= 0) return FAILED;
    
    char buffer[BUFFER_LEN];
    recv_bytes = read(sockfd, buffer, BUFFER_LEN);
    if ( recv_bytes<=0 ) return FAILED;
    buffer[recv_bytes] = '\0';
   
    if (strcmp(buffer, "FOUND") != 0) return FAILED;
        //Receive filesize
        int fileSize;
        recv_bytes = read(sockfd, &fileSize, sizeof(fileSize));
        if (recv_bytes <= 0) return FAILED;
        char *filename = basename(file);
        printf("File Name Len: %ld\nFile name: %s\nFile Size: %d\n", strlen(filename), filename, fileSize);
        
        //open file
        int fd = open(filename, O_CREAT | O_WRONLY, 0666);
        if (fd < 0) {
            perror("[-] file opening Failed\n");
            return FAILED;
        }
        //receive data
        int total_bytes = 0;
        while(total_bytes < fileSize) {
            recv_bytes = read(sockfd, buffer, BUFFER_LEN);
            send_bytes = write(fd, buffer, recv_bytes);
            if (send_bytes != recv_bytes) return FAILED;
            total_bytes += recv_bytes;
        }
        //close file
        close(fd);
    
   
    
    return SUCCESS;
}

int main() {

    //Socket Creation
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-] Socket Creation Failed\n");
        exit(EXIT_FAILURE);
    }

    //Connecting to server
    int status;
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CONNECTION_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(CONNECTION_IP);
    serverAddr.sin_zero[8] = '\0';

    status = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (status < 0) {
        perror("[-] Connection to Server Failed\n");
        exit(EXIT_FAILURE);
    }

    while(1) {
        int choice;
        printf("\nMenu:\n");
        printf("1. Send file to server\n");
        printf("2. Request file from server\n");
        printf("3. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        if (Send_Cmd(sockfd, choice) != SUCCESS) {
            printf("[-] Command Sending Failed\nRetry.....");
            continue;
        }

        switch(choice) {
            case 1:
            {
                if (Send_File(sockfd) == SUCCESS) printf("[+] File Sent Successfully\n");
                else printf("[-] Failed to send...Retry\n");
            }
            break;
            case 2:
            {
                if (Recv_File(sockfd) == SUCCESS) printf("[+] File Received Successfully\n");
                else printf("[-] Failed to Receive...Retry\n");
            }
            break;
            case 3:
            close(sockfd);
            exit(0);
        }
    }

    close(sockfd);   
    return 0;
}