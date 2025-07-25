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

    
    //Socket Option setting
    int status;
    int opt = 1;
    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (status < 0) {
        perror("[-] Socket Option setting Failed\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket Option Setting Succesfull\n");

    //Binding Socket
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CONNECTION_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(CONNECTION_IP);
    serverAddr.sin_zero[8] = '\0';

    status = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (status < 0) {
        perror("[-] Error in binding socket\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket Binding Successfull\n");
    printf("[+] Socket created on %s:%d\n", inet_ntoa(serverAddr.sin_addr), CONNECTION_PORT);
    
    //Listening to Socket
    status = listen(sockfd, 2);
    if (status < 0) {
        perror("[-] Error in Listening Socket\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Listening.....\n");

    //Connection to multiple clients using fork
    pid_t childPID;
    char buffer[BUFFER_LEN];
    int newSocket;
    struct sockaddr_in newAddr;
    socklen_t newLen = sizeof(newAddr);

    while(1) {
        newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &newLen);
        if (newSocket < 0) {
            perror("[-] Connection Failed\n");
            continue;
        }

        printf("Connection accepted from Client %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        childPID = fork(); //Creating to child to listen to accepted client
        if (childPID == 0) { //Child Process running
            close(sockfd);

            //Communication with Client
            while(1) {
                int recv_bytes = read(newSocket, buffer, BUFFER_LEN);
                if(recv_bytes <= 0) {
                    printf("[+] Client %s:%d Disconnected\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
                    close(newSocket);
                    exit(0);
                }
                else {
                    buffer[recv_bytes] = '\0';
                    printf("Client: %s\n",buffer);
                    if (strcmp(buffer, "exit") == 0) {
                        printf("[+] Client %s:%d Disconnected\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));                     
                        close(newSocket);
                        exit(0);
                    }
                    write(newSocket, buffer, recv_bytes);
                    bzero(buffer, BUFFER_LEN);
                }
                
            }
        }
    }

    return 0;
}