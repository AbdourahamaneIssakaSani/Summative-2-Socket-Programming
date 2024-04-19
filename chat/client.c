#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

void *send_messages(void *arg);
void *receive_messages(void *arg);

/**
 * main - Entry point of the program
*/
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    pthread_t send_thread, receive_thread;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, send_messages, (void *)&sock) != 0) {
        perror("pthread_create for send_messages failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&sock) != 0) {
        perror("pthread_create for receive_messages failed");
        exit(EXIT_FAILURE);
    }

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    close(sock);

    return 0;
}

/**
 * send_messages - Send messages to the server
 * @arg: The socket file descriptor
 * Return: NULL
 */
void *send_messages(void *arg) {
    int sock = *((int *)arg);
    char buffer[1024] = {0};

    while(1) {
        printf("\nClient: ");
        fgets(buffer, sizeof(buffer), stdin);
        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
    }

    return NULL;
}

/**
 * receive_messages - Receive messages from the server
 * @arg: The socket file descriptor
 * Return: NULL
 */
void *receive_messages(void *arg) {
    int sock = *((int *)arg);
    char buffer[1024] = {0};

    while(1) {
        if (recv(sock, buffer, sizeof(buffer), 0) <= 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }
        printf("\nServer: %s", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    return NULL;
}
