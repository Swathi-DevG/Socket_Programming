#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define CONNECTION_PORT 3500
#define BUFFER_SIZE 100

int sock_fd, client_sockfd;

void *Send_Message(void *arg) {
    char msg[BUFFER_SIZE];
    int sent_bytes;

    while(1) {
        printf("You: ");
        fflush(stdout);

        if (fgets(msg, BUFFER_SIZE, stdin) == NULL) {
            perror("Error reading input");
            continue;
        }

        msg[strcspn(msg, "\n")] = '\0';  // Remove newline

        if (strcmp(msg, "exit") == 0) {
            printf("Disconnecting...\n");
            close(client_sockfd);
            close(sock_fd);
            exit(0);
        }

        sent_bytes = write(client_sockfd, msg, strlen(msg));
        if (sent_bytes < 0) {
            perror("Error sending message");
            break;
        }
    }
    return NULL;
}

void *Recv_Message(void *arg) {
    char recv_buf[BUFFER_SIZE];
    int bytes_read;

    while(1) {
        bytes_read = read(client_sockfd, recv_buf, sizeof(recv_buf)-1);
        if (bytes_read <= 0) {
            printf("\nClient disconnected.\n");
            close(client_sockfd);
            close(sock_fd);
            exit(0);
        }
        recv_buf[bytes_read] = '\0';
        printf("\nClient: %s\nYou: ", recv_buf);
        fflush(stdout);
    }
    return NULL;
}

int main() {
    // Socket Creation
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Socket Option Setting
    int status; 
    int option_val = 1;
    status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(option_val));
    if (status < 0) {
        perror("Socket option setting failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Binding the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONNECTION_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_zero[8] = '\0';

    status = bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (status < 0) {
        perror("Socket binding failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Listening for connections
    status = listen(sock_fd, 5);
    if (status < 0) {
        perror("Listening failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for connections on port %d...\n", CONNECTION_PORT);

    // Accepting connection
    struct sockaddr_in client_addr;
    int client_len;
    
    client_sockfd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sockfd < 0) {
        perror("Accept failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client connected!\n");

    pthread_t T1, T2;
    status = pthread_create(&T1, NULL, Send_Message, NULL);
    if (status != 0) {
        perror("Send thread creation failed");
        close(client_sockfd);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    status = pthread_create(&T2, NULL, Recv_Message, NULL);
    if (status != 0) {
        perror("Receive thread creation failed");
        close(client_sockfd);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    pthread_join(T1, NULL);
    pthread_join(T2, NULL);

    close(client_sockfd);
    close(sock_fd);
    return 0;
}