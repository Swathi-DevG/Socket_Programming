#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define CONNECTION_PORT 4457
#define BUFFER_LEN      1024
#define FRAME_DATA      1
#define FRAME_ACK       0

typedef struct {
    char packet[BUFFER_LEN];
    int frame_type;
    int seq_no;
    int ack;
}Frame; 

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

    //Communication
    int frame_id = 0;      // To set sequence number
    Frame frame_send;      
    Frame frame_recv;
    
    int recv_bytes;
    struct sockaddr_in client_addr;
    len = sizeof(client_addr);

    while(1) {
        recv_bytes = recvfrom(sockfd, &frame_recv, sizeof(frame_recv), 0, (struct sockaddr *)&client_addr, &len);
        if (recv_bytes > 0 && frame_recv.frame_type == FRAME_DATA) {
            frame_id %= 2;
            if (frame_recv.seq_no == frame_id) {
                printf("Frame %d Recieved: %s\n", frame_id, frame_recv.packet);
                frame_send.frame_type = FRAME_ACK;
                frame_send.seq_no = 0;
                frame_send.ack = frame_recv.seq_no +1;
                sendto(sockfd, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&client_addr, len);
                printf("-> ACK Sent for %d\n", frame_recv.seq_no);   
                frame_id ++;
            }
            else {
                printf("-> Duplicate Frame Received\n");
                sendto(sockfd, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&client_addr, len);
                printf("-> Resent ACK for %d\n", frame_recv.seq_no);   
            }
            
        }
    }

    close(sockfd);
    return 0;
}