#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define CONNECTION_PORT 4457
#define CONNECTION_IP   "127.0.0.1"
#define BUFFER_LEN      1024
#define NAME_LEN        20


void *Send_Msg(void *arg) {

    int sockfd = *((int *)arg);
    char buffer[BUFFER_LEN];
    int send_bytes; 

    while(1) {
        printf("You: ");
        fflush(stdout);

        fgets(buffer, BUFFER_LEN, stdin);
        buffer[strcspn(buffer, "\n")] ='\0';

        send_bytes = write(sockfd, buffer, strlen(buffer));
        if (send_bytes < 0) {
            perror("[-] Error in sending data\n");
            break;
        }
        if(strcmp(buffer, "exit") == 0) {
            printf("[+] Disconnecting to Client.....\n");
            close(sockfd);
            exit(0);
        }
    }
    pthread_exit(NULL);
}

void *Recv_Msg(void *arg) {

    int sockfd = *((int *)arg);
    char buffer[BUFFER_LEN+NAME_LEN];
    int recv_bytes; 
   
    while(1) {
        recv_bytes = read(sockfd, buffer, sizeof(buffer));
        if(recv_bytes <= 0) {
            printf("[-] Client Disconnected\n");
            close(sockfd);
            exit(0);
        }
        buffer[recv_bytes] = '\0';
        printf("\n%s\nYou: ", buffer);
        fflush(stdout);
        if (strcmp(buffer, "exit") == 0) {
            printf("[-] Client Disconnected\n");
            close(sockfd);
            exit(0);
        }

    }
    pthread_exit(NULL);
}
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

    signal(SIGCHLD, SIG_IGN); //To prevent Zombie processes

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
            pthread_t send_id, recv_id;
            status = pthread_create(&send_id, NULL, Send_Msg, &newSocket);
            if (status != 0) {
                perror("Send thread creation failed");
                close(newSocket);
                exit(EXIT_FAILURE);
            }

            status = pthread_create(&recv_id, NULL, Recv_Msg, &newSocket);
            if (status != 0) {
                perror("Receive thread creation failed");
                close(newSocket);
                exit(EXIT_FAILURE);
            }

            pthread_join(send_id, NULL);
            pthread_join(recv_id, NULL);
            exit(0); 
        }
    }

    return 0;
}