//works normallyy

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8888
#define CHUNK_SIZE 10
#define MAX_CHUNKS 100
#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 100000 // 0.1 seconds

struct Packet {
    int seq_num;
    int total_chunks;
    char data[CHUNK_SIZE];
};

void die(char *s) {
    perror(s);
    exit(1);
}

int send_message(int sockfd, struct sockaddr_in *cliaddr, const char *message) {
    struct Packet packet;
    int total_chunks = (strlen(message) + CHUNK_SIZE - 1) / CHUNK_SIZE;
    
    printf("Sending message with %d chunks\n", total_chunks);

    for (int i = 0; i < total_chunks; i++) {
        packet.seq_num = i;
        packet.total_chunks = total_chunks;
        strncpy(packet.data, message + i * CHUNK_SIZE, CHUNK_SIZE);

        while (1) {
            if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)cliaddr, sizeof(*cliaddr)) < 0) {
                die("sendto() failed");
            }
            printf("Sent chunk %d/%d: %s\n", i + 1, total_chunks, packet.data);

            fd_set readfds;
            struct timeval tv;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            tv.tv_sec = TIMEOUT_SEC;
            tv.tv_usec = TIMEOUT_USEC;

            int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

            if (activity < 0) {
                die("select() error");
            } else if (activity == 0) {
                printf("Timeout for chunk %d, retransmitting\n", i + 1);
                continue;
            }

            int ack;
            if (recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL) < 0) {
                die("recvfrom() failed");
            }

            if (ack == i) {
                printf("Received ACK for chunk %d\n", i + 1);
                break;
            }
        }
    }
    return 0;
}

int receive_message(int sockfd, struct sockaddr_in *cliaddr, char *buffer, int buffer_size) {
    struct Packet packet;
    int received_chunks = 0;
    int total_chunks = 0;
    char *chunks[MAX_CHUNKS];

    while (1) {
        socklen_t len = sizeof(*cliaddr);
        if (recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)cliaddr, &len) < 0) {
            die("recvfrom() failed");
        }

        printf("Received chunk %d: %s\n", packet.seq_num + 1, packet.data);

        if (total_chunks == 0) {
            total_chunks = packet.total_chunks;
            printf("Expecting %d chunks\n", total_chunks);
        }

        chunks[packet.seq_num] = strdup(packet.data);
        received_chunks++;

        // Send ACK
        if (sendto(sockfd, &packet.seq_num, sizeof(packet.seq_num), 0, (struct sockaddr *)cliaddr, sizeof(*cliaddr)) < 0) {
            die("sendto() failed");
        }
        printf("Sent ACK for chunk %d\n", packet.seq_num + 1);

        if (received_chunks == total_chunks) {
            break;
        }
    }

    // Reconstruct the message
    buffer[0] = '\0';
    for (int i = 0; i < total_chunks; i++) {
        strncat(buffer, chunks[i], buffer_size - strlen(buffer) - 1);
        free(chunks[i]);
    }

    return 0;
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        die("socket creation failed");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        die("bind failed");
    }

    printf("Server listening on port %d\n", PORT);

    char message[1000];
    char response[1000];

    while (1) {
        printf("Waiting for message...\n");
        receive_message(sockfd, &cliaddr, message, sizeof(message));
        printf("Received message: %s\n", message);

        printf("Enter response: ");
        fgets(response, sizeof(response), stdin);
        response[strcspn(response, "\n")] = 0;  // Remove newline

        send_message(sockfd, &cliaddr, response);
    }

    close(sockfd);
    return 0;
}