#define _DEFAULT_SOURCE

/*
 * Team ownership:
 * - Team Member 1: owns the Sleeping IT Support simulation, queue logic,
 *   ticket arrival handling, and sleep/wake-up synchronization.
 * - Team Member 2: supports shared synchronization style and test alignment.
 * - Team Member 3: reviews integration, edge cases, and final test output.
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define SPECIALIST_COUNT 2
#define QUEUE_SIZE 5
#define DEFAULT_TICKET_COUNT 12

typedef struct {
    int ticket_id;
} ticket_job_t;

static ticket_job_t waiting_queue[QUEUE_SIZE];
static int queue_front = 0;
static int queue_rear = 0;
static int queue_count = 0;

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ticket_available = PTHREAD_COND_INITIALIZER;
static int production_done = 0;

static int is_queue_full(void) {
    return queue_count == QUEUE_SIZE;
}

static int is_queue_empty(void) {
    return queue_count == 0;
}

static void enqueue_ticket(int ticket_id) {
    waiting_queue[queue_rear].ticket_id = ticket_id;
    queue_rear = (queue_rear + 1) % QUEUE_SIZE;
    queue_count++;
}

static int dequeue_ticket(int *ticket_id) {
    if (is_queue_empty()) {
        return 0;
    }

    *ticket_id = waiting_queue[queue_front].ticket_id;
    queue_front = (queue_front + 1) % QUEUE_SIZE;
    queue_count--;
    return 1;
}

static void *specialist_worker(void *arg) {
    long specialist_id = (long)(intptr_t)arg;

    /* Team Member 1: specialist sleep/wake-up loop. */
    for (;;) {
        int ticket_id = -1;

        pthread_mutex_lock(&queue_mutex);
        while (is_queue_empty() && !production_done) {
            printf("[Specialist %ld] Queue empty, going to sleep.\n", specialist_id);
            pthread_cond_wait(&ticket_available, &queue_mutex);
        }

        if (is_queue_empty() && production_done) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        if (dequeue_ticket(&ticket_id)) {
            printf("[Specialist %ld] Picked ticket %d from queue.\n", specialist_id, ticket_id);
        }

        pthread_mutex_unlock(&queue_mutex);

        if (ticket_id >= 0) {
            printf("[Specialist %ld] Processing ticket %d.\n", specialist_id, ticket_id);
            usleep(120000);
            printf("[Specialist %ld] Finished ticket %d.\n", specialist_id, ticket_id);
        }
    }

    printf("[Specialist %ld] No more tickets, shutting down.\n", specialist_id);
    return NULL;
}

static void *ticket_worker(void *arg) {
    long ticket_id = (long)(intptr_t)arg;

    /* Team Member 1: ticket arrival and queue admission. */
    usleep((ticket_id % 4 + 1) * 70000);

    pthread_mutex_lock(&queue_mutex);
    if (is_queue_full()) {
        printf("[Ticket %ld] Queue full, ticket dropped.\n", ticket_id);
    } else {
        enqueue_ticket((int)ticket_id);
        printf("[Ticket %ld] Added to queue. Waiting count: %d.\n", ticket_id, queue_count);
        pthread_cond_signal(&ticket_available);
    }
    pthread_mutex_unlock(&queue_mutex);

    return NULL;
}

static int read_ticket_total(void) {
    int ticket_total = DEFAULT_TICKET_COUNT;
    int scanned_count = scanf("%d", &ticket_total);

    if (scanned_count != 1 || ticket_total < 1) {
        ticket_total = DEFAULT_TICKET_COUNT;
    }

    return ticket_total;
}

int main(void) {
    int ticket_total = read_ticket_total();
    pthread_t specialist_threads[SPECIALIST_COUNT];
    pthread_t *ticket_threads = calloc((size_t)ticket_total, sizeof(*ticket_threads));

    if (ticket_threads == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    printf("Sleeping IT Support simulation started with %d tickets.\n", ticket_total);

    /* Team Member 1: launch specialist threads. */
    for (long specialist_id = 0; specialist_id < SPECIALIST_COUNT; ++specialist_id) {
        pthread_create(&specialist_threads[specialist_id], NULL, specialist_worker, (void *)(intptr_t)specialist_id);
    }

    /* Team Member 1: launch ticket threads. */
    for (long ticket_id = 0; ticket_id < ticket_total; ++ticket_id) {
        pthread_create(&ticket_threads[ticket_id], NULL, ticket_worker, (void *)(intptr_t)ticket_id);
    }

    for (int iter_a = 0; iter_a < ticket_total; ++iter_a) {
        pthread_join(ticket_threads[iter_a], NULL);
    }

    pthread_mutex_lock(&queue_mutex);
    production_done = 1;
    pthread_cond_broadcast(&ticket_available);
    pthread_mutex_unlock(&queue_mutex);

    for (int iter_b = 0; iter_b < SPECIALIST_COUNT; ++iter_b) {
        pthread_join(specialist_threads[iter_b], NULL);
    }

    free(ticket_threads);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&ticket_available);

    return 0;
}
