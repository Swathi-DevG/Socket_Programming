#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define CONNECTION_PORT 4457

int main()  {

    //create socket
    int sockfd;
    sockfd  =  socket(AF_INET,  SOCK_DGRAM, 0);
    if (sockfd < 0)  {
        perror("Failed to create Socket\n");
        exit(EXIT_FAILURE);
    }

    //Setting socket options
    int status;
    int opt_val = 1;
    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
    if (status < 0) {
        perror("Socket Option setting Failed\n");
        exit(EXIT_FAILURE);
    }

    //Bind socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONNECTION_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_zero[8] = '\0';

    status = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    if (status < 0) {
        perror("Socket Binding Failed\n");
        exit(EXIT_FAILURE);
    }

    //Printing Server Address
    struct sockaddr_in bound_addr;
    socklen_t len = sizeof(bound_addr);
    getsockname(sockfd, (struct sockaddr*)&bound_addr, &len);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &bound_addr.sin_addr, ip, sizeof(ip));
    printf("Server is listening on: %s:%d\n", ip, ntohs(bound_addr.sin_port));

    //Receive from server
    char recv_buf[100];
    struct sockaddr_in client_addr;
    len = sizeof(client_addr);

    int recv_bytes = recvfrom(sockfd, recv_buf, sizeof(recv_buf)-1, 0, (struct sockaddr *)&client_addr, &len);
    if (recv_bytes <= 0) {
        perror("recvfrom failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    recv_buf[recv_bytes] = '\0';
    printf("Message from client: %s\n", recv_buf);
    //Printing Client Address
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
    printf("Client Address: %s:%d\n", ip, ntohs(client_addr.sin_port));
    
    //send to client
    char *msg = "Hello from Server";
    int send_bytes = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&client_addr, len);
    if (send_bytes <= 0) {
        perror("sendto failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    return 0;
}