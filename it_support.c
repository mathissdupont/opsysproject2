#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>

#define SPECIALIST_COUNT 2
#define QUEUE_SIZE 5
#define TICKET_COUNT 15

// Global queue variables
int waiting_queue[QUEUE_SIZE];
int queue_front = 0;
int queue_rear = 0;
int queue_count = 0;

// Synchronization primitives
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t waiting_tickets;

// Exit flag for clean shutdown
short should_exit = 0;

// Ticket counters
int total_created_tickets = TICKET_COUNT;
int accepted_tickets = 0;
int dropped_tickets = 0;
int completed_tickets = 0;

// Completion signal for accepted tickets
sem_t completed_ticket_signal;

// Helper function: check if queue is full
short is_queue_full() {
    return queue_count == QUEUE_SIZE;
}

// Helper function: check if queue is empty
short is_queue_empty() {
    return queue_count == 0;
}

// Helper function: add ticket to queue (assumes queue is not full)
void enqueue_ticket(int ticket_id) {
    waiting_queue[queue_rear] = ticket_id;
    queue_rear = (queue_rear + 1) % QUEUE_SIZE;
    queue_count++;
}

// Helper function: remove ticket from queue (assumes queue is not empty)
int dequeue_ticket() {
    int ticket_id = waiting_queue[queue_front];
    queue_front = (queue_front + 1) % QUEUE_SIZE;
    queue_count--;
    return ticket_id;
}

// Customer/ticket thread: creates and submits a ticket
void* customer_thread(void* arg) {
    int ticket_id = (intptr_t)arg;
    
    // Lock the queue before checking and modifying
    pthread_mutex_lock(&queue_mutex);
    
    if (!is_queue_full()) {
        // Queue has space: enqueue the ticket
        enqueue_ticket(ticket_id);
        accepted_tickets++;
        printf("[TICKET %d] ACCEPTED and queued (Queue size: %d/%d)\n", 
               ticket_id, queue_count, QUEUE_SIZE);
        
        // Unlock queue and signal that a ticket is available
        pthread_mutex_unlock(&queue_mutex);
        sem_post(&waiting_tickets);
    } else {
        // Queue is full: drop the ticket
        dropped_tickets++;
        printf("[TICKET %d] DROPPED - Queue is FULL!\n", ticket_id);
        pthread_mutex_unlock(&queue_mutex);
    }
    
    return NULL;
}

// IT specialist thread: processes tickets from the queue
void* specialist_thread(void* arg) {
    int specialist_id = (intptr_t)arg;
    
    for(;;) {
        // Wait for a ticket to be available (blocks if queue is empty)
        sem_wait(&waiting_tickets);
        
        // Lock the queue before accessing it
        pthread_mutex_lock(&queue_mutex);
        
        // Check if we should exit
        if (should_exit && is_queue_empty()) {
            printf("[SPECIALIST %d] Exiting.\n", specialist_id);
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        
        // Dequeue a ticket if one is available
        if (!is_queue_empty()) {
            int ticket_id = dequeue_ticket();
            printf("[SPECIALIST %d] WOKE UP and dequeued ticket %d (Queue size: %d)\n", 
                   specialist_id, ticket_id, queue_count);
            
            // Unlock queue before processing (allows other threads to access queue)
            pthread_mutex_unlock(&queue_mutex);
            
            // Process the ticket (simulate work)
            printf("[SPECIALIST %d] PROCESSING ticket %d...\n", specialist_id, ticket_id);
            sleep(2);
            printf("[SPECIALIST %d] COMPLETED ticket %d\n", specialist_id, ticket_id);

            pthread_mutex_lock(&queue_mutex);
            completed_tickets++;
            pthread_mutex_unlock(&queue_mutex);
            sem_post(&completed_ticket_signal);
        } else {
            // Queue is empty but not exiting yet
            pthread_mutex_unlock(&queue_mutex);
        }
    }

    return NULL;
}

int main() {
    pthread_t specialist_threads[SPECIALIST_COUNT];
    pthread_t customer_threads[TICKET_COUNT];
    
    // Initialize semaphore (initially 0 tickets available)
    sem_init(&waiting_tickets, 0, 0);
    sem_init(&completed_ticket_signal, 0, 0);
    
    printf("========================================\n");
    printf("   IT SUPPORT SYNCHRONIZATION SYSTEM\n");
    printf("========================================\n");
    printf("Configuration:\n");
    printf("  Specialists: %d\n", SPECIALIST_COUNT);
    printf("  Queue Size: %d\n", QUEUE_SIZE);
    printf("  Total Tickets: %d\n\n", TICKET_COUNT);
    
    // Create specialist threads (they will wait for tickets)
    printf("[MAIN] Creating %d specialist threads...\n", SPECIALIST_COUNT);
    for (int iter_a = 0; iter_a < SPECIALIST_COUNT; iter_a++) {
        pthread_create(&specialist_threads[iter_a], NULL, specialist_thread, (void*)(intptr_t)iter_a);
    }
    printf("[MAIN] Specialists created and waiting for tickets.\n\n");
    
    // Create customer threads with staggered arrivals
    printf("[MAIN] Creating %d customer threads...\n", TICKET_COUNT);
    for (int iter_b = 0; iter_b < TICKET_COUNT; iter_b++) {
        pthread_create(&customer_threads[iter_b], NULL, customer_thread, (void*)(intptr_t)iter_b);
        usleep(20000);
    }
    printf("[MAIN] All customer threads created.\n\n");
    
    // Wait for all customer threads to complete
    for (int iter_c = 0; iter_c < TICKET_COUNT; iter_c++) {
        pthread_join(customer_threads[iter_c], NULL);
    }
    
    printf("\n========================================\n");
    printf("[MAIN] All tickets have been submitted.\n");
    printf("[MAIN] Waiting for %d accepted tickets to complete...\n", accepted_tickets);
    printf("========================================\n\n");
    
    // Wait until all accepted tickets are completed
    for (int iter_a = 0; iter_a < accepted_tickets; iter_a++) {
        sem_wait(&completed_ticket_signal);
    }
    
    // Signal specialists to exit gracefully
    pthread_mutex_lock(&queue_mutex);
    should_exit = 1;
    pthread_mutex_unlock(&queue_mutex);
    
    // Wake up all waiting specialists so they can check the should_exit flag
    for (int iter_a = 0; iter_a < SPECIALIST_COUNT; iter_a++) {
        sem_post(&waiting_tickets);
    }
    
    // Wait for all specialist threads to complete
    for (int iter_a = 0; iter_a < SPECIALIST_COUNT; iter_a++) {
        pthread_join(specialist_threads[iter_a], NULL);
    }
    
    // Clean up all resources
    pthread_mutex_destroy(&queue_mutex);
    sem_destroy(&waiting_tickets);
    sem_destroy(&completed_ticket_signal);
    
    printf("\n========================================\n");
    printf("   IT SUPPORT SYSTEM COMPLETED\n");
    printf("========================================\n");
    printf("Total tickets created: %d\n", total_created_tickets);
    printf("Accepted tickets: %d\n", accepted_tickets);
    printf("Dropped tickets: %d\n", dropped_tickets);
    printf("Completed tickets: %d\n", completed_tickets);
    
    return 0;
}
