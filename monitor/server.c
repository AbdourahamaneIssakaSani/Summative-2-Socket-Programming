#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

void print_opened_files(const char *buffer) {
    char *buffer_copy = strdup(buffer);
    if (buffer_copy == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    char *delim = "\n";
    char *token = strtok(buffer_copy, delim);
    while (token != NULL) {
        printf("Received URL: %s\n", token);
        token = strtok(NULL, delim);
    }

    free(buffer_copy);
}

void *receive_messages(void *arg) {
    int new_socket = *(int *)arg;
    char buffer[1024] = {0};

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if (recv(new_socket, buffer, sizeof(buffer), 0) <= 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }
        print_opened_files(buffer);
    }

    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&new_socket) != 0) {
        perror("pthread_create for receive_messages failed");
        exit(EXIT_FAILURE);
    }

    pthread_join(receive_thread, NULL);

    return 0;
}
