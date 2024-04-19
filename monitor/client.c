#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/inotify.h>

#define PORT 8080
#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))
#define MAX_FILES 100
#define MAX_FILE_SIZE 100

char opened_files[MAX_FILES][MAX_FILE_SIZE];
int opened_files_count = 0;

// TODO: clean steps and add docs
// help: man inotify

void *send_messages(void *arg);
void *track_activity(void *arg);
void send_opened_files(int sock);

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    pthread_t track_thread, send_thread;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&track_thread, NULL, track_activity, (void *)&sock) != 0) {
        perror("pthread_create for track_activity failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, send_messages, (void *)&sock) != 0) {
        perror("pthread_create for send_messages failed");
        exit(EXIT_FAILURE);
    }

    pthread_join(track_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sock);

    return 0;
}

void send_opened_files(int sock) {
    char buffer[1024];
    for (int i = 0; i < opened_files_count; ++i) {
        strcat(buffer, opened_files[i]);
        strcat(buffer, "\n");
    }
    send(sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    opened_files_count = 0;
}

void *send_messages(void *arg) {
    int sock = *((int *)arg);

    while (1) {
        send_opened_files(sock);
        sleep(3);
    }

    return NULL;
}

void *track_activity(void *arg) {
    int sock = *((int *)arg);

    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
    }

    int wd = inotify_add_watch(inotify_fd, "/home/abissa", IN_OPEN | IN_MOVED_TO | IN_CREATE);

    char buffer[BUF_LEN] = {0};
    int length, i = 0;
    char *activity;

    while (1) {
        i = 0;
        length = read(inotify_fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
        }

        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len) {

                     // check if the file name already exists in the array
                    int exists = 0;
                    for (int j = 0; j < opened_files_count; j++) {
                        if (strcmp(opened_files[j], event->name) == 0) {
                            exists = 1;
                            break;
                        }
                    }
                    // printf("File opened: %s\n", event->name);
                    if (exists == 0 && opened_files_count < MAX_FILES) {
                        strcpy(opened_files[opened_files_count], event->name);
                        opened_files_count++;
                    }



                i += EVENT_SIZE + event->len;
            }
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    return NULL;
}


