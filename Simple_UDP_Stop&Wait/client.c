#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define CONNECTION_PORT     4457
#define BUFFER_LEN          1024
#define FRAME_DATA          1
#define FRAME_ACK           0
#define RETRIES_FINISHED    3

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

    //Timeout Mechanism
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    int status = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (status < 0) {
        perror("Socket Setting Option failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }


    //send to Server
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONNECTION_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    server_addr.sin_zero[8] = '\0';
    
    //Communication
    int ack_recv = 1;      // To track the prev frame sent or not
    int frame_id = 0;      // To set sequence number
    Frame frame_send;      
    Frame frame_recv;

    int recv_bytes;
    int retries = 0;

    while(1) {
        if (ack_recv) {
            frame_id %=2;
            frame_send.frame_type = FRAME_DATA;
            frame_send.seq_no = frame_id;
            frame_send.ack = 0;
            printf("Enter data to send: ");
            fgets(frame_send.packet, BUFFER_LEN, stdin);
            frame_send.packet[strcspn(frame_send.packet, "\n")] = '\0';

            if (strcmp(frame_send.packet, "exit") == 0) {
                printf("Disconnecting: ");
                close(sockfd);
                exit(0);
            }

            sendto(sockfd, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&server_addr, len);
            printf("Frame %d is sent\n", frame_id);
        }
        
        recv_bytes = recvfrom(sockfd, &frame_recv, sizeof(frame_recv), 0, (struct sockaddr *)&server_addr, &len);
        if (recv_bytes > 0 && frame_recv.seq_no == 0 && frame_recv.ack == frame_send.seq_no+1) {
            printf("-> ACK received for Packet %d\n",frame_id);
            ack_recv = 1;
            frame_id++;
            retries = 0;
        }
        else {
            printf("-> NO ACK received for Packet %d\n", frame_id);
            if (retries++ >= RETRIES_FINISHED) {
                printf("Server is Unreachable.....\n");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            sendto(sockfd, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&server_addr, len);
            printf("Resending Frame %d is sent\n", frame_id);
            ack_recv = 0;
        }
    }

    close(sockfd);
    return 0;  

}