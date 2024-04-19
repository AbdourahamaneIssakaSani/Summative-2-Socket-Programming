#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>


/*set a port you want or free the default one using lsof and kill*/
#define PORT 8080

// TODO: make send_messages and receive_messages functions reusable
// after release of Apr 20th if possible

void *send_messages(void *arg);
void *receive_messages(void *arg);


/**
 * main - Entry point of the program
 * Return: 0 on success
 */
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t receive_thread, send_thread;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Create separate threads for sending and receiving messages for each client
        if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&new_socket) != 0) {
            perror("pthread_create for receive_messages failed");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&send_thread, NULL, send_messages, (void *)&new_socket) != 0) {
            perror("pthread_create for send_messages failed");
            exit(EXIT_FAILURE);
        }
    }

    close(server_fd);

    return 0;
}


/**
 * send_messages - Send messages to the client
 * @arg: The client socket
 * Return: NULL
 */
void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024] = {0};
    ssize_t valread;

    while ((valread = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        printf("\nClient: %s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    if (valread == 0) {
        printf("Client disconnected.\n");
    } else {
        perror("recv failed");
    }

    close(client_socket);
    return NULL;
}

/**
 * send_messages - Send messages to the client
 * @arg: The client socket
 * Return: NULL
 */
void *send_messages(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024] = {0};

    while(1) {
        printf("\nServer: ");
        fgets(buffer, sizeof(buffer), stdin);
        send(client_socket, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
    }

    return NULL;
}