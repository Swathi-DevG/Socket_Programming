#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define CONNECTION_PORT 3500

int main() {

    //Socket Creation
    int sock_fd;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket Creation Failed\n");
        exit(EXIT_FAILURE);
    }

    //Socket Option Setting
    int option_val = 1;
    int status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(option_val));
    if (status < 0) {
        perror("Socket Option Setting Failed\n");
        exit(EXIT_FAILURE);
    }

    //Binding the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONNECTION_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_zero[8] = '\0';
 
    status = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    if (status < 0) {
        perror("Socket Binding Failed\n");
        exit(EXIT_FAILURE);
    }

    //Listening to specific port(Marking socket as passive)
    status = listen(sock_fd, 4);
    if (status < 0) {
        perror("Listening for Connections Failed\n");
        exit(EXIT_FAILURE);
    }

    //Accepting connection
    int client_sockfd;
    struct sockaddr_in client_address;
    int len_of_addr;

    client_sockfd = accept(sock_fd, (struct sockaddr *)&client_address, (socklen_t *)&len_of_addr);
    if (client_sockfd < 0) {
        perror("Connection with Client Failed\n");
        exit(EXIT_FAILURE);
    }
    
    //Receive Data From Client
    char recv_buf[80]; 
    int bytes_read = read(client_sockfd, recv_buf, sizeof(recv_buf));
    recv_buf[bytes_read] = '\0';
    printf("Message from Client: %s\n",recv_buf);

    //Send data to Client
    char *send_buf = "Hi!, This is message from Server";
    int sent_bytes = write(client_sockfd, send_buf, strlen(send_buf));
    if (sent_bytes < 0) {
        perror("Error in sending data to client\n");
    }

    //closing all sockets created
    close(sock_fd);
    close(client_sockfd);

    return 0;
}