# Problem 1: Sleeping IT Support

## 1. Problem Definition

The Sleeping IT Support problem is a classic producer-consumer synchronization scenario. The problem simulates a real-world IT helpdesk system where:

- Multiple ticket/customer threads arrive randomly and submit support tickets.
- A limited number of IT specialists are available to process these tickets.
- A bounded waiting queue (capacity = 5) temporarily holds incoming tickets.
- When the queue is full, new tickets are rejected and dropped.
- When no tickets are available, IT specialists sleep until a new ticket arrives.
- The goal is to ensure safe concurrent access to shared resources (the queue) while preventing race conditions, deadlocks, and data corruption.

This problem tests core operating system concepts: mutual exclusion, synchronization primitives (semaphores and mutexes), and producer-consumer coordination.

---

## 2. Thread Architecture

The program implements two types of threads:

### 2.1 IT Specialist Threads (Consumers)

- **Count**: 2 specialists (SPECIALIST_COUNT)
- **Role**: Wait for and process tickets from the queue
- **Behavior**:
  - Sleep when the queue is empty
  - Wake up when a new ticket arrives
  - Dequeue one ticket at a time
  - Process the ticket safely
  - Repeat until no more tickets are available

### 2.2 Ticket/Customer Threads (Producers)

- **Count**: 15 tickets (TICKET_COUNT)
- **Role**: Create and submit tickets to the queue
- **Behavior**:
  - Arrive and attempt to enqueue a ticket
  - Check if the queue has space
  - Either accept and queue the ticket, or drop it
  - Complete immediately after submission attempt

### 2.3 Main Thread (Orchestrator)

- Initializes synchronization primitives
- Creates all specialist threads
- Creates all customer threads
- Waits for all work to complete
- Cleans up resources

---

## 3. Shared Resources and Synchronization Primitives

### 3.1 Global Waiting Queue

```c
int waiting_queue[QUEUE_SIZE];      // Holds ticket IDs
int queue_front = 0;                // Index of first ticket
int queue_rear = 0;                 // Index for next ticket
int queue_count = 0;                // Current number of tickets
#define QUEUE_SIZE 5                // Maximum capacity
```

The queue is implemented as a circular buffer. The `queue_count` variable is critical because it represents the current queue size and is used to:

- Check if the queue is full: `queue_count == QUEUE_SIZE`
- Check if the queue is empty: `queue_count == 0`
- Prevent overflow and underflow conditions

### 3.2 Synchronization Primitives

**Mutex (pthread_mutex_t queue_mutex)**

- Protects all read and write operations on the queue
- Ensures only one thread can modify queue data at a time
- Prevents race conditions when multiple threads access `queue_front`, `queue_rear`, and `queue_count` simultaneously

**Semaphore (sem_t waiting_tickets)**

- Tracks the number of tickets available for processing
- Initialized to 0 (no tickets initially)
- Specialists wait (sem_wait) when no tickets are available
- Incremented (sem_post) each time a ticket is accepted into the queue
- Enables efficient sleep/wake-up: specialists block on the semaphore without busy-waiting

**Exit Flag (short should_exit)**

- Used for clean shutdown
- Set to 1 after all customer threads complete
- Signals specialists to stop processing and exit

---

## 4. IT Specialist Thread Implementation

### 4.1 Thread Lifecycle

Each specialist thread executes the following loop:

```c
void* specialist_thread(void* arg) {
    int specialist_id = (intptr_t)arg;

    for(;;) {
        // Step 1: Wait for a ticket to be available
        sem_wait(&waiting_tickets);

        // Step 2: Lock the queue
        pthread_mutex_lock(&queue_mutex);

        // Step 3: Check exit condition
        if (should_exit && is_queue_empty()) {
            pthread_mutex_unlock(&queue_mutex);
            break;  // Exit thread
        }

        // Step 4: Safely dequeue a ticket
        if (!is_queue_empty()) {
            int ticket_id = dequeue_ticket();
            printf("[SPECIALIST %d] WOKE UP and dequeued ticket %d\n",
                   specialist_id, ticket_id);

            // Step 5: Unlock queue before processing (critical!)
            pthread_mutex_unlock(&queue_mutex);

            // Step 6: Process the ticket (hold no locks)
            printf("[SPECIALIST %d] PROCESSING ticket %d...\n", specialist_id, ticket_id);
            sleep(1);  // Simulate work
            printf("[SPECIALIST %d] COMPLETED ticket %d\n", specialist_id, ticket_id);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    return NULL;
}
```

### 4.2 Key Design Decisions

**Why sem_wait() First?**

- Specialists block on the semaphore before acquiring the mutex
- This prevents busy-waiting and reduces CPU usage
- Only when a ticket is signaled does the specialist acquire the lock

**Why Unlock Before Processing?**

- The mutex is released immediately after dequeuing
- Processing happens outside the critical section
- This allows other threads to enqueue new tickets while one is being processed
- Maximizes parallelism and throughput

**Why Check `should_exit && is_queue_empty()`?**

- Ensures a specialist exits only when no more tickets will arrive AND the queue is empty
- Prevents premature termination while work remains

---

## 5. Ticket/Customer Thread Implementation

Each ticket thread executes once:

```c
void* customer_thread(void* arg) {
    int ticket_id = (intptr_t)arg;

    // Lock the queue
    pthread_mutex_lock(&queue_mutex);

    if (!is_queue_full()) {
        // Queue has space: accept and enqueue
        enqueue_ticket(ticket_id);
        printf("[TICKET %d] ACCEPTED and queued (Queue size: %d/%d)\n",
               ticket_id, queue_count, QUEUE_SIZE);

        // Unlock and signal a specialist
        pthread_mutex_unlock(&queue_mutex);
        sem_post(&waiting_tickets);
    } else {
        // Queue is full: drop the ticket
        printf("[TICKET %d] DROPPED - Queue is FULL!\n", ticket_id);
        pthread_mutex_unlock(&queue_mutex);
    }

    return NULL;
}
```

### 5.1 Ticket Processing Steps

1. **Lock the queue** - Prevent concurrent modifications
2. **Check capacity** - Is `queue_count < QUEUE_SIZE`?
3. **If space available**:
   - Enqueue the ticket
   - Print acceptance message
   - Unlock the queue
   - Signal the semaphore to wake a sleeping specialist
4. **If queue full**:
   - Print dropped message
   - Unlock the queue
   - Return (ticket is lost)

### 5.2 Why Signal After Unlock?

The order is important:

```c
pthread_mutex_unlock(&queue_mutex);  // Release lock first
sem_post(&waiting_tickets);          // Then signal
```

This prevents a waking specialist from immediately blocking on the mutex the producer still holds. Although modern implementations handle this efficiently, this order reduces unnecessary context switching.

---

## 6. Waiting Queue Design

### 6.1 Circular Buffer Implementation

The queue uses a circular buffer to efficiently reuse memory:

```c
void enqueue_ticket(int ticket_id) {
    waiting_queue[queue_rear] = ticket_id;
    queue_rear = (queue_rear + 1) % QUEUE_SIZE;  // Wrap around
    queue_count++;
}

int dequeue_ticket() {
    int ticket_id = waiting_queue[queue_front];
    queue_front = (queue_front + 1) % QUEUE_SIZE;  // Wrap around
    queue_count--;
    return ticket_id;
}
```

The modulo operator (%) ensures indices wrap from `QUEUE_SIZE-1` back to 0, eliminating the need to shift elements.

### 6.2 Why queue_count is Critical

`queue_count` is the single source of truth for queue fullness. Without it:

- Determining fullness requires complex index comparisons
- Race conditions could cause incorrect queue_full() checks
- Multiple read-modify-write cycles increase timing windows for errors

With `queue_count`:

- Fullness check: `queue_count == QUEUE_SIZE` (atomic-like when protected by mutex)
- Emptiness check: `queue_count == 0`
- Provides visibility into queue utilization in output messages

---

## 7. Mutex Usage for Preventing Race Conditions

### 7.1 Protected Critical Section

The mutex protects all access to:

- `waiting_queue[]`
- `queue_front`
- `queue_rear`
- `queue_count`

### 7.2 Example Race Condition (Without Mutex)

Suppose two tickets arrive simultaneously:

```
Thread A: Check if queue is full          queue_count = 5
Thread B: Check if queue is full          queue_count = 5
Thread A: Enqueue ticket, queue_count++   queue_count = 6 (OVERFLOW!)
Thread B: Enqueue ticket, queue_count++   queue_count = 7 (OVERFLOW!)
```

The queue is corrupted because both threads read the same `queue_count` value before either incremented it.

### 7.3 With Mutex Protection

```
Thread A: Lock mutex
          Check if queue is full (queue_count = 5)
          Queue is full, unlock, drop ticket
Thread B: Lock mutex (was blocked)
          Check if queue is full (queue_count = 5)
          Queue is full, unlock, drop ticket
```

Each thread sees the correct state because they execute serially within the critical section.

### 7.4 Mutex Placement Rules in This Program

1. **Lock BEFORE** any queue access
2. **Unlock IMMEDIATELY AFTER** dequeuing (before processing)
3. **Unlock BEFORE** signaling the semaphore (avoid unnecessary blocking)
4. **Lock AGAIN** for every new queue access (even in a loop)

---

## 8. Semaphore Usage for Sleep/Wake-Up

### 8.1 Semaphore Mechanics

The semaphore `waiting_tickets` maintains a counter:

```c
sem_init(&waiting_tickets, 0, 0);  // Start with 0 tickets available
```

- **sem_wait()**: Decrements counter. If counter is 0, thread blocks (sleeps) until signaled.
- **sem_post()**: Increments counter and wakes one blocked thread.

### 8.2 Sleep/Wake-Up Sequence

**Scenario: Specialist arrives before any tickets**

```
Specialist: sem_wait(&waiting_tickets)  // Counter = 0
            Blocks and sleeps (no CPU consumption)

Customer:   pthread_mutex_lock(&queue_mutex)
            enqueue_ticket()
            pthread_mutex_unlock(&queue_mutex)
            sem_post(&waiting_tickets)  // Counter incremented to 1

Specialist: Wakes up, continues execution
            Acquires mutex, dequeues ticket, processes
```

**Scenario: Multiple tickets arrive**

```
Specialist 1: sem_wait() blocks, sleeps
Specialist 2: sem_wait() blocks, sleeps

Customer 1: sem_post()  // Wakes Specialist 1
Customer 2: sem_post()  // Wakes Specialist 2
Customer 3: sem_post()  // Counter incremented to 1 (no specialist sleeping)

Specialist 1: Wakes, processes Customer 1's ticket
Specialist 2: Wakes, processes Customer 2's ticket
              sem_wait() blocks, sleeps (Customer 3's ticket waits)

Specialist 1: Finishes, sem_wait() returns immediately
              (Counter = 1 from Customer 3), dequeues ticket
```

### 8.3 Efficiency

Semaphores are more efficient than busy-waiting:

- **Busy-wait** (without semaphore):
  ```c
  while (is_queue_empty()) {
      sleep(10ms);  // Waste CPU checking periodically
  }
  ```
- **Semaphore** (with sem_wait):
  ```c
  sem_wait(&waiting_tickets);  // Blocked, consumes no CPU until signaled
  ```

---

## 9. Full Queue Behavior

When `queue_count == QUEUE_SIZE`:

### 9.1 Detection

```c
if (!is_queue_full()) {
    // Accept ticket
} else {
    // Reject ticket
    printf("[TICKET %d] DROPPED - Queue is FULL!\n", ticket_id);
}
```

### 9.2 What Happens

1. The customer thread does NOT enqueue the ticket
2. The customer thread does NOT signal the semaphore
3. The ticket is immediately lost
4. A "DROPPED" message is printed with timestamp info
5. The customer thread exits

### 9.3 Why This is Correct

- Prevents queue overflow and data corruption
- Simulates real systems that reject new requests during peak load
- Output clearly shows which tickets were dropped
- Allows analysis of system capacity vs. load

---

## 10. Why Two Specialists Cannot Process the Same Ticket

### 10.1 Mutual Exclusion

The mutex ensures only one specialist can execute `dequeue_ticket()` at a time:

```c
pthread_mutex_lock(&queue_mutex);
int ticket_id = dequeue_ticket();  // Atomic from specialist's perspective
pthread_mutex_unlock(&queue_mutex);
```

### 10.2 Dequeue Atomicity

Once a specialist calls `dequeue_ticket()`:

1. The specialist reads `ticket_id` from `waiting_queue[queue_front]`
2. Increments `queue_front` (with wrap-around)
3. Decrements `queue_count`
4. Returns the ticket_id

No other specialist can interfere because the mutex prevents concurrent dequeue operations. The ticket is removed from the queue and assigned exclusively to one specialist.

### 10.3 Output Proof

Each ticket has exactly one "WOKE UP and dequeued" message, one "PROCESSING" message, and one "COMPLETED" message—all from the same specialist ID. This proves no sharing occurred.

---

## 11. Deadlock Prevention

### 11.1 Potential Deadlock Scenario (Avoided)

A deadlock could occur if:

- Specialist holds mutex and waits on semaphore
- Another specialist can't proceed because mutex is held

Our implementation prevents this by **unlocking before waiting**:

```c
// WRONG (could deadlock):
pthread_mutex_lock(&queue_mutex);
sem_wait(&waiting_tickets);  // Waits while holding lock!
dequeue();
pthread_mutex_unlock(&queue_mutex);

// CORRECT (no deadlock):
sem_wait(&waiting_tickets);   // Wait without holding lock
pthread_mutex_lock(&queue_mutex);
dequeue();
pthread_mutex_unlock(&queue_mutex);
```

### 11.2 Lock Ordering

The program follows a consistent lock order:

1. Always wait on semaphore first
2. Then acquire mutex
3. Release mutex immediately after critical section

This prevents circular wait conditions.

### 11.3 No Circular Dependencies

- Specialists depend on semaphore and mutex, but no circular dependencies
- Customers only use mutex, no circular dependencies
- No thread holds multiple locks simultaneously
- All locks are released within a bounded time

### 11.4 Graceful Shutdown

The `should_exit` flag allows clean termination:

```c
pthread_mutex_lock(&queue_mutex);
should_exit = 1;  // Signal shutdown
pthread_cond_broadcast(&ticket_available);
pthread_mutex_unlock(&queue_mutex);

for (int iter_a = 0; iter_a < SPECIALIST_COUNT; iter_a++) {
    sem_post(&waiting_tickets);  // Wake all waiting specialists
}
```

This ensures no specialist is stuck waiting indefinitely.

---

## 12. Output Demonstration

### 12.1 Expected Output Structure

The program output clearly shows the complete lifecycle:

```
========================================
   IT SUPPORT SYNCHRONIZATION SYSTEM
========================================
Configuration:
  Specialists: 2
  Queue Size: 5
  Total Tickets: 15

[MAIN] Creating 2 specialist threads...
[MAIN] Specialists created and waiting for tickets.

[MAIN] Creating 15 customer threads...
[TICKET 0] ACCEPTED and queued (Queue size: 1/5)
[SPECIALIST 0] WOKE UP and dequeued ticket 0 (Queue size: 0)
[SPECIALIST 0] PROCESSING ticket 0...
[TICKET 1] ACCEPTED and queued (Queue size: 1/5)
[SPECIALIST 1] WOKE UP and dequeued ticket 1 (Queue size: 0)
[SPECIALIST 1] PROCESSING ticket 1...
... (more tickets)
[TICKET 5] ACCEPTED and queued (Queue size: 5/5)
[TICKET 6] DROPPED - Queue is FULL!
[TICKET 7] DROPPED - Queue is FULL!
... (specialists process tickets)
[SPECIALIST 0] COMPLETED ticket 0
[SPECIALIST 0] WOKE UP and dequeued ticket 2 (Queue size: 4)
... (more processing)

========================================
[MAIN] All tickets have been submitted.
[MAIN] Waiting for specialists to finish...
========================================

[SPECIALIST 0] COMPLETED ticket 12
[SPECIALIST 1] COMPLETED ticket 13
[SPECIALIST 0] Exiting.
[SPECIALIST 1] Exiting.

========================================
   IT SUPPORT SYSTEM COMPLETED
========================================
```

### 12.2 Key Observable Behaviors

| Event                   | Output Evidence                                    | Meaning                                         |
| ----------------------- | -------------------------------------------------- | ----------------------------------------------- |
| **Ticket Accepted**     | `[TICKET X] ACCEPTED and queued (Queue size: Y/5)` | Queue had space, ticket added safely            |
| **Ticket Dropped**      | `[TICKET X] DROPPED - Queue is FULL!`              | Queue at capacity, rejection occurred           |
| **Specialist Sleeping** | Specialists inactive until tickets arrive          | Efficient use of resources (no busy-waiting)    |
| **Specialist Wake-up**  | `[SPECIALIST X] WOKE UP and dequeued ticket Y`     | Semaphore signaled specialist; queue had ticket |
| **Processing**          | `[SPECIALIST X] PROCESSING ticket Y...`            | Specialist safely owns and processes ticket     |
| **Completion**          | `[SPECIALIST X] COMPLETED ticket Y`                | Ticket fully processed, specialist available    |
| **No Duplication**      | Each ticket has exactly one "WOKE UP" message      | Mutual exclusion prevents double-processing     |

### 12.3 Synchronization Proof from Output

1. **Mutual Exclusion Works**: Each ticket appears in exactly one specialist's output sequence.
2. **Queue Capacity Enforced**: Dropped tickets appear only when queue is full (5/5).
3. **No Deadlock**: Specialists complete processing and exit cleanly.
4. **Fairness**: Both specialists get opportunities to process tickets.
5. **Correctness**: Total completed tickets + dropped tickets = total tickets submitted.

---

## 13. Summary

The Sleeping IT Support program demonstrates core operating system synchronization principles:

- **Mutex** protects shared queue data from race conditions
- **Semaphore** enables efficient sleep/wake-up coordination
- **Circular buffer** efficiently manages bounded queue
- **Critical sections** are minimized to unlock quickly
- **Atomic operations** (dequeue) prevent duplicate processing
- **Exit signaling** ensures clean shutdown and resource cleanup
- **Output** provides clear evidence of correct synchronization behavior

The implementation is production-quality with careful attention to avoiding deadlocks, race conditions, and resource leaks while maintaining high efficiency and clarity.
