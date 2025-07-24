#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define CONNECTION_PORT 4457
#define CONNECTION_IP   "127.0.0.1"
#define BUFFER_LEN      1024

int main() {

    //Socket Creation
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-] Socket Creation Failed\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket Creation Success\n");

    //Socket Connection to Server
    int status;
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CONNECTION_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(CONNECTION_IP);
    serverAddr.sin_zero[8] = '\0';

    status = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr));
    if (status < 0) {
        perror("[-] Connection to Server Failed\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Connected to Server Successfully\n");

    //Communcation
    char buffer[BUFFER_LEN];
    while(1) {
        printf("Client: ");
        fgets(buffer, BUFFER_LEN-1, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        write(sockfd, buffer, strlen(buffer));
        if (strcmp(buffer, "exit") == 0) {
            printf("[+] Disconnecting....\n");
            close(sockfd);
            exit(0);
        }

        int recv_bytes = read(sockfd, buffer, BUFFER_LEN);
        if(recv_bytes <= 0) {
            printf("[-] Error in Receiving\n");
        }
        else {
            buffer[recv_bytes] = '\0';
            printf("Server: %s\n",buffer);
        }
    }
    
    close(sockfd);
    return(0);
}