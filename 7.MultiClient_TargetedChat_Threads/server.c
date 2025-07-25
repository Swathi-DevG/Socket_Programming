#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#define CONNECTION_PORT 4457
#define CONNECTION_IP   "127.0.0.1"
#define MAX_CLIENTS     10
#define BUFFER_LEN      1024
#define NAME_LEN        20

typedef struct {
    int sockfd;
    char name[NAME_LEN];
}client_t;

client_t client_info[MAX_CLIENTS];
int client_count;
pthread_mutex_t lock;
   
void Remove_Client(int sockfd) {
    for (int i=0; i<client_count; i++) {
        if(client_info[i].sockfd = sockfd) {
            printf("[-] Client %s Disconnected...\n", client_info[i].name);
            close(sockfd);
            client_info[i] = client_info[client_count - 1];
            client_count--;
            break;
        }
    }
}
void *Send_Msg(void *arg) {
    char buffer[BUFFER_LEN];
    int send_bytes; 
    char name[NAME_LEN];

    while(1) {
        printf("Enter Client name or All to Broadcast: ");
        fflush(stdout);
        fgets(name, NAME_LEN, stdin);
        name[strcspn(name, "\n")] = '\0';
        
        //Broadcasting
        if(strcmp(name, "All") == 0) {
            pthread_mutex_lock(&lock);
            printf("Send to %s: ", name);
            fgets(buffer, BUFFER_LEN, stdin);
            buffer[strcspn(buffer, "\n")] = '\0';
            for (int i=0; i<client_count; i++) {
                send_bytes = write(client_info[i].sockfd, buffer, strlen(buffer));
                if (send_bytes < 0) {
                    perror("[-] Error in sending data\n");
                }
            }
            pthread_mutex_unlock(&lock);
            continue;
        }

        //Targeted Thread
        pthread_mutex_lock(&lock);
        int found = 0;
        for (int i=0; i<client_count; i++) {
            if(strcmp(client_info[i].name, name) == 0) {
                printf("Send to %s: ", name);
                fgets(buffer, BUFFER_LEN, stdin);
                buffer[strcspn(buffer, "\n")] = '\0';

                send_bytes = write(client_info[i].sockfd, buffer, strlen(buffer));
                if (send_bytes < 0) {
                    perror("[-] Error in sending data\n");
                }
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&lock);

        if (!found) {
            printf("%s Client NOT found\n", name);
        }
    }
    pthread_exit(NULL);
}

void *Recv_Msg(void *arg) {

    int sockfd = *((int *)arg);
    free(arg);
    char buffer[BUFFER_LEN+NAME_LEN+1];
    int recv_bytes; 
   
    //Receiving and storing client name
    char name[NAME_LEN];
    recv_bytes = read(sockfd, name, NAME_LEN);
    if (recv_bytes <= 0) {
        printf("[-] Client Disconnected\n");
        close(sockfd);
        exit(0);
    }
    name[recv_bytes] = '\0';

    pthread_mutex_lock(&lock);
    if (client_count < MAX_CLIENTS) {
        client_info[client_count].sockfd = sockfd;
        strcpy(client_info[client_count].name, name);
        client_count++;
    }
    pthread_mutex_unlock(&lock);

    printf("[+] %s Client Connected\n", name);

    while(1) {
        recv_bytes = read(sockfd, buffer, sizeof(buffer));
        if(recv_bytes <= 0) {
            pthread_mutex_lock(&lock);
            Remove_Client(sockfd);
            pthread_mutex_unlock(&lock);
            exit(0);
        }
        buffer[recv_bytes] = '\0';
        printf("\n%s\nEnter Client name: ", buffer);
        fflush(stdout);
        if (strcmp(buffer, "exit") == 0) {
            pthread_mutex_lock(&lock);
            Remove_Client(sockfd);
            pthread_mutex_unlock(&lock);
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

    pthread_t send_id;
    status = pthread_create(&send_id, NULL, Send_Msg, NULL);
    if (status != 0) {
        perror("Send thread creation failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    pthread_detach(send_id);  
    
    while(1) {
        newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &newLen);
        if (newSocket < 0) {
            perror("[-] Connection Failed\n");
            continue;
        }

        printf("Connection accepted from Client %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        //Communication with Client
        pthread_t recv_id;
        int *dup = malloc(sizeof(int));
        *dup = newSocket;
        status = pthread_create(&recv_id, NULL, Recv_Msg, dup);
        if (status != 0) {
            perror("Receive thread creation failed");
            close(newSocket);
            exit(EXIT_FAILURE);
        }
        pthread_detach(recv_id);
        
    }
    close(sockfd);

    return 0;
}