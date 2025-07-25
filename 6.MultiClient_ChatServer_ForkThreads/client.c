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

char name[NAME_LEN];
int sockfd;

void *Send_Msg(void *arg) {
    char msg[BUFFER_LEN];
    char buffer[NAME_LEN+BUFFER_LEN+1];
    int send_bytes; 

    while(1) {
        printf("You: ");
        fflush(stdout);

        fgets(msg, BUFFER_LEN, stdin);
        msg[strcspn(msg, "\n")] ='\0';

        sprintf(buffer, "%s: %s",name, msg);
        send_bytes = write(sockfd, buffer, strlen(buffer));
        if (send_bytes < 0) {
            perror("[-] Error in sending data\n");
            break;
        }
        if(strcmp(msg, "exit") == 0) {
            printf("[+] Disconnecting.....\n");
            close(sockfd);
            exit(0);
        }
    }
    pthread_exit(NULL);
}

void *Recv_Msg(void *arg) {
    char buffer[BUFFER_LEN];
    int recv_bytes; 

    while(1) {
        recv_bytes = read(sockfd, &buffer, BUFFER_LEN);
        if(recv_bytes <= 0) {
            printf("\n[-] Server disconnected.\n");
            close(sockfd);
            exit(0);
        }
        buffer[recv_bytes] = '\0';
        printf("\nServer: %s\nYou: ", buffer);
        fflush(stdout);
    }
    pthread_exit(NULL);
}

int main() {

    //Naming Client for multiClient
    printf("Enter name for Client: ");
    fgets(name, NAME_LEN, stdin);
    name[strcspn(name, "\n")] = '\0'; 
    
    //Socket Creation
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
    pthread_t send_id, recv_id;
    status = pthread_create(&send_id, NULL, Send_Msg, NULL);
    if (status != 0) {
        perror("Send thread creation failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    status = pthread_create(&recv_id, NULL, Recv_Msg, NULL);
    if (status != 0) {
        perror("Receive thread creation failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_join(send_id, NULL);
    pthread_join(recv_id, NULL);
    
    close(sockfd);
    return(0);
}