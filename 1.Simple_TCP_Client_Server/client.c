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

    //Connection to Socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONNECTION_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_zero[8] = '\0';

    int status = connect(sock_fd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(struct sockaddr));
    if (status < 0) {
        perror("Connection to Server Failed\n");
        exit(EXIT_FAILURE);
    }

    //Send data to server
    char *send_buf = "Hello!, This is message from Client";
    int sent_bytes = write(sock_fd, send_buf, strlen(send_buf));
    if (sent_bytes < 0) {
        perror("Error in sending data to server\n");
    }

    //Receive data from server
    char recv_buf[80]; 
    int bytes_read = read(sock_fd, recv_buf, sizeof(recv_buf));
    recv_buf[bytes_read] = '\0';
    printf("Message from Server: %s\n",recv_buf);

    //Close socket
    close(sock_fd);
    
    return 0;
}