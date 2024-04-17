#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_WEIGHT 660
#define NUM_FLOORS 8
#define DOOR_TIME 30
#define LOAD_OFFLOAD_TIME 20

void print_floor(int floor);
void open_door();
void close_door();
int can_enter(int weight);
void *input_handler(void *arg);
void move_to_floor(int target_floor);
void *floor_request_handler(void *arg);

int current_floor = 0;
int current_weight = 0;
int passenger_weights[NUM_FLOORS] = {0, 0, 0, 0, 0, 0, 0, 0};

/**
 * Reprssents [calling_floor][dest_floor]
 * 0 means no request, 1 means request
*/
int floor_requests[NUM_FLOORS][NUM_FLOORS] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**
 * main - Entry point of the program
 * Return: 0 on success
*/
int main() {
    pthread_t input_thread, request_thread;
    pthread_create(&input_thread, NULL, input_handler, NULL);
    pthread_create(&request_thread, NULL, floor_request_handler, NULL);

    pthread_join(request_thread, NULL);
    pthread_join(input_thread, NULL);

    return 0;
}

/**
 * print_floor - Print the current floor the elevator has arrived at
 * @floor: The floor number
*/
void print_floor(int floor) {
    printf("Elevator arrived at floor %d\n", floor);
}

/**
 * open_door - Simulate the elevator door opening
 *
 * @note: Because the condition in the rubric is the elevator waits
 *      for 20seconds at each floor to load and/or offload,
 *      the door opening time is set to 10 seconds to simulate
 *      and the same for the door closing time. Total time spent
 *     will then be 20 seconds for loading and offloading
*/
void open_door() {
    printf("Door opening...\n");
    sleep(10);
}

/**
 * close_door - Simulate the elevator door closing
 *
 * @note: Because the condition in the rubric is the elevator waits
 *      for 20seconds at each floor to load and/or offload,
 *      the door opening time is set to 10 seconds to simulate
 *      and the same for the door closing time. Total time spent
 *     will then be 20 seconds for loading and offloading
*/
void close_door() {
    printf("Door closing...\n");
    sleep(10);
}

/**
 * can_enter - Check if the elevator can accept more passengers
 * @weight: The weight of the new passenger
 * Return: 1 if the passenger can enter, 0 otherwise
*/
int can_enter(int weight) {
    return (current_weight + weight) <= MAX_WEIGHT;
}

/**
 * input_handler - Handle the input from the user to call the elevator
 * @arg: The argument passed to the thread
*/
void *input_handler(void *arg) {
    while (1) {
        int key;
        printf("Press 9 to call the elevator: ");
        scanf("%d", &key);
        if (key != 9) {
            printf("Invalid input. Please press '9' to call the elevator.\n");
            continue;
        }

        int current_floor, weight, next_floor;
        printf("Enter current floor, weight, and next floor number: ");
        scanf("%d %d %d", &current_floor, &weight, &next_floor);

        if (current_floor < 0 || current_floor >= NUM_FLOORS) {
            printf("Invalid current floor. Please enter a floor between 0 and %d.\n", NUM_FLOORS - 1);
            continue;
        }

        pthread_mutex_lock(&mutex);
        if (!can_enter(weight)) {
            printf("Elevator at full capacity. Cannot accept more passengers.\n");
            pthread_mutex_unlock(&mutex);
            continue;
        }

        floor_requests[current_floor][next_floor] = 1;
        passenger_weights[next_floor] += weight;
        pthread_cond_signal(&cond); // Signal elevator thread
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}


/**
 * move_to_floor - Move the elevator to the target floor
 * @target_floor: The floor number to move to
 *
 * @note: To simulate moving to the next floor, the program waits for 4 seconds
 *      before moving to the next floor. This function will also check for any
 *      requests at the current floor and service them before moving to the target floor.
*/
void move_to_floor(int target_floor) {
    close_door();

    /*Determine if we are moving up or down*/
    int increment = (target_floor > current_floor) ? 1 : -1;

    while (current_floor != target_floor) {
        current_floor += increment;
        print_floor(current_floor);
        sleep(4);

        pthread_mutex_lock(&mutex);

        /*Check if there is a request at this floor*/
        for (int j = 0; j < NUM_FLOORS; j++) {
            if (floor_requests[current_floor][j] == 1) {
                open_door();
                close_door();

                /*clear the request as it's been serviced*/
                floor_requests[current_floor][j] = 0;

                /*adjust the target floor if the new request has a higher priority*/
                if ((increment == 1 && j > target_floor) || (increment == -1 && j < target_floor)) {
                    target_floor = j;
                }

                /* Remove weight of passengers at their destination floor */
                current_weight -= passenger_weights[j];
                printf("Passenger(s) offloaded at floor %d. Remaining weight capacity: %d\n", j, MAX_WEIGHT - current_weight);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    open_door();
    close_door();
}

/**
 * floor_request_handler - Handle the requests from the passengers to move to a specific floor
 * @arg: The argument passed to the thread
 *
 * @note: This function will check for any requests from the passengers and move the elevator to the
 *      requested floor
*/
void *floor_request_handler(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        int next_floor = -1;

        /*Find the next floor to service from the current position*/
        for (int i = 0; i < NUM_FLOORS; i++) {
            for (int j = 0; j < NUM_FLOORS; j++) {
                if (floor_requests[i][j] == 1 && next_floor == -1) {
                    next_floor = j; // Select this as the next floor to move to
                    floor_requests[i][j] = 0; // Clear request after processing
                }
            }
        }

        pthread_mutex_unlock(&mutex);

        if (next_floor != -1 && next_floor != current_floor) {
            move_to_floor(next_floor);
        } else {
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond, &mutex); // Wait for a condition signal when there's a new request
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

