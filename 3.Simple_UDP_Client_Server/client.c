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

    //send to Server
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONNECTION_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    server_addr.sin_zero[8] = '\0';
    
    char *msg = "Hello from Client";
    int send_bytes = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&server_addr, len);
    if (send_bytes <= 0) {
        perror("Error in sending message\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //Receive from server
    char recv_buf[100];

    int recv_bytes = recvfrom(sockfd, recv_buf, sizeof(recv_buf)-1, 0, (struct sockaddr *)&server_addr, &len);
    if (recv_bytes <= 0) {
        perror("recvfrom failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    recv_buf[recv_bytes] = '\0';
    printf("Message from Server: %s\n", recv_buf);
    //Printing Server Address
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, ip, sizeof(ip));
    printf("Server Address: %s:%d\n", ip, ntohs(server_addr.sin_port));
    

    close(sockfd);
    return 0;  

}