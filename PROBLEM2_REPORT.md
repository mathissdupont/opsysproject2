# Problem 2: AI Researchers and GPUs

## 1. Problem Definition

The AI Researchers and GPUs problem is a classic Dining Philosophers variant where:

- **RESEARCHER_COUNT** (5) researchers compete for **GPU_COUNT** (5) GPUs
- Each researcher needs exactly 2 adjacent GPUs to train models
- Researcher i uses GPUs i and (i+1) % GPU_COUNT
- Multiple researchers might request overlapping GPU pairs simultaneously
- Goal: Prevent **deadlock**, **starvation**, and ensure **safe concurrent access**

This is one of the classic Operating Systems problems because it naturally creates a circular wait condition:
- Researcher 0 waits for GPU 0 and GPU 1
- Researcher 1 waits for GPU 1 and GPU 2
- ...
- Researcher 4 waits for GPU 4 and GPU 0

If all researchers acquire their first GPU simultaneously, each holds one GPU while waiting for another—creating a **circular dependency** and **deadlock**.

---

## 2. Global Architecture

### 2.1 Shared Resources

```c
static pthread_mutex_t gpu_mutexes[GPU_COUNT];           // 5 mutexes for GPUs
static pthread_mutex_t availability_mutex;               // For atomic checks
static short gpu_available[GPU_COUNT] = {1,1,1,1,1};   // Availability flags
static sem_t room_limit;                                 // Semaphore for room limiting
```

### 2.2 Helper Functions

```c
static int left_gpu_of(int researcher_id)   // returns researcher_id
static int right_gpu_of(int researcher_id)  // returns (researcher_id + 1) % GPU_COUNT
static void acquire_two_mutexes(first, second)   // Acquires in order
static void release_two_mutexes(first, second)   // Releases in order
```

### 2.3 Four Scenarios

The program implements **four different synchronization strategies**, each addressing the deadlock problem differently:

1. **Scenario 1: Maximum n-1 Researchers** (Team Member 2)
   - Uses a counting semaphore initialized to 4 (n-1)
   - At most 4 out of 5 researchers can enter the "room" simultaneously
   - Mathematically guarantees at least one researcher can always progress

2. **Scenario 2: GPU Availability Check** (Team Member 2)
   - Uses atomic check-and-reserve: both GPUs must be free before acquisition
   - Avoids lock ordering entirely; uses a separate availability array
   - Ensures no deadlock through "all-or-nothing" acquisition

3. **Scenario 3: Asymmetric Resource Acquisition** (Team Member 3 - THIS FILE)
   - **Even-numbered researchers** acquire right GPU first, then left GPU
   - **Odd-numbered researchers** acquire left GPU first, then right GPU
   - Breaks the circular wait condition by enforcing different acquisition orders

4. **Scenario 4: Right-GPU Preference** (Team Member 3 - THIS FILE)
   - Most researchers acquire left GPU first, then right GPU
   - **Researcher 2 (special)** acquires right GPU first, then left GPU
   - Uses `pthread_mutex_trylock()` for the second GPU—if it fails, releases the first and retries
   - Eliminates blocking wait chains through non-blocking semantics

---

## 3. Scenario 3: Asymmetric Resource Acquisition (Team Member 3)

### 3.1 Algorithm Overview

```c
static void *asymmetric_worker(void *arg)
{
    int researcher_id = ...;
    int left_gpu = left_gpu_of(researcher_id);
    int right_gpu = right_gpu_of(researcher_id);
    
    // Determine acquisition order based on parity
    int first_gpu = (researcher_id % 2 == 0) ? right_gpu : left_gpu;
    int second_gpu = (researcher_id % 2 == 0) ? left_gpu : right_gpu;

    for (int iter_a = 0; iter_a < TRAINING_ROUNDS; ++iter_a)
    {
        simulate_thinking(researcher_id);
        
        acquire_two_mutexes(first_gpu, second_gpu);  // Acquire in order
        simulate_training(researcher_id, "asymmetric order");
        release_two_mutexes(first_gpu, second_gpu);  // Release in reverse
    }
    
    return NULL;
}
```

### 3.2 Deadlock-Free Guarantee

**Key Insight: Breaking the Circular Wait Condition**

In the original Dining Philosophers problem, all philosophers (researchers) follow the same acquisition order:
1. Pick up left fork (GPU)
2. Pick up right fork (GPU)

With 5 researchers sharing 5 GPUs in a ring, if all researchers simultaneously acquire their left GPU, each holds one GPU while waiting for its neighbor's—creating a deadlock.

**Asymmetric Solution Mechanism:**

By alternating the acquisition order, we break the circular dependency chain:

```
Researcher 0 (even):   right_gpu(1) → left_gpu(0)
Researcher 1 (odd):    left_gpu(1)  → right_gpu(2)
Researcher 2 (even):   right_gpu(3) → left_gpu(2)
Researcher 3 (odd):    left_gpu(3)  → right_gpu(4)
Researcher 4 (even):   right_gpu(0) → left_gpu(4)
```

**Why This Prevents Deadlock:**

The circular wait condition requires:
- T0 holds resource R0 and waits for R1
- T1 holds resource R1 and waits for R2
- ...
- Tn holds resource Rn and waits for R0

With asymmetric ordering, **the cycle is broken** because:
- Even and odd researchers acquire in opposite orders
- This asymmetry means at least one researcher can always progress through both GPUs
- A formal proof uses the concept of a **total resource ordering**: because not all threads follow the same acquisition order, we cannot have a complete circular dependency

**Correctness Proof:**

Suppose a deadlock occurs. Then there exists a cycle of researchers T₀, T₁, ..., Tₖ where:
- T₀ holds GPU A and waits for GPU B
- T₁ holds GPU B and waits for GPU C
- ...
- Tₖ holds GPU Z and waits for GPU A

Each Tᵢ follows one of two orderings: either (left, right) or (right, left). For a cycle to exist, there must be researchers with both orderings in the cycle. But the asymmetric nature ensures that a researcher with a different ordering can eventually proceed because:
- If researcher n tries to acquire in the opposite order of neighbor n-1, they won't deadlock
- This breaks the chain at that point

### 3.3 Edge Cases

#### Edge Case 1: Two Adjacent Researchers with Opposite Parities

**Scenario:**
- Researcher 0 (even) wants: GPU 1 → GPU 0
- Researcher 1 (odd) wants: GPU 1 → GPU 2

Both want GPU 1, but they approach from different directions.
- If Researcher 0 gets GPU 1 first, it then gets GPU 0 (success)
- Researcher 1 waits on GPU 1; when released by 0, it gets GPU 1 then GPU 2 (success)

**No deadlock because:** One researcher always makes complete progress before the other acquires the shared GPU.

#### Edge Case 2: All 5 Researchers Waking Simultaneously

**Scenario:** All researchers finish thinking at the same time and seek 2 GPUs.

With 5 researchers and 5 GPUs:
- Even researchers (0, 2, 4) try to acquire right-first
- Odd researchers (1, 3) try to acquire left-first

At least one thread will successfully acquire its first GPU without blocking indefinitely because the asymmetric ordering prevents a perfect circular wait chain from forming.

#### Edge Case 3: GPU Fragmentation

**Scenario:** All researchers hold exactly one GPU but none can acquire a second GPU.

This cannot happen with our algorithm because:
- Mutex lock is atomic—either you get the GPU or you don't
- The second `acquire_two_mutexes()` call will either succeed immediately or block
- If blocked, no partial acquisition state persists

### 3.4 Performance Notes

- **Lock Contention:** With 5 researchers and 5 GPUs, contention is moderate. In practice, even-numbered researchers rarely collide with odd-numbered ones on the same GPU pair.
- **Fairness:** The algorithm does not guarantee FIFO access, but it prevents starvation. A researcher might be delayed, but will eventually acquire both GPUs.
- **Training Rounds:** Set to 3 (`TRAINING_ROUNDS`) to allow sufficient demonstration in test output.

---

## 4. Scenario 4: Right-GPU Preference (Team Member 3)

### 4.1 Algorithm Overview

```c
static void *right_preference_worker(void *arg)
{
    int researcher_id = ...;
    int left_gpu = left_gpu_of(researcher_id);
    int right_gpu = right_gpu_of(researcher_id);
    int special_researcher_id = 2;

    for (int iter_a = 0; iter_a < TRAINING_ROUNDS; ++iter_a)
    {
        // Normal order: left first
        int first_gpu = left_gpu;
        int second_gpu = right_gpu;

        // Exception: Researcher 2 prefers right first
        if (researcher_id == special_researcher_id)
        {
            first_gpu = right_gpu;
            second_gpu = left_gpu;
        }

        simulate_thinking(researcher_id);

        // Retry loop: try to acquire both GPUs
        for (;;)
        {
            pthread_mutex_lock(&gpu_mutexes[first_gpu]);  // Block until 1st available
            
            if (pthread_mutex_trylock(&gpu_mutexes[second_gpu]) == 0)
            {
                // Success: both GPUs acquired
                printf("[Researcher %d] Acquired GPUs %d and %d with preference.\n", ...);
                simulate_training(researcher_id, "preferred-order mutexes");
                
                pthread_mutex_unlock(&gpu_mutexes[second_gpu]);
                pthread_mutex_unlock(&gpu_mutexes[first_gpu]);
                break;  // Exit retry loop; go to next training round
            }

            // Failed: release first GPU and retry
            pthread_mutex_unlock(&gpu_mutexes[first_gpu]);
            usleep(35000);  // Brief wait before retry
        }
    }
    
    return NULL;
}
```

### 4.2 Deadlock-Free Guarantee

**Key Insight: Non-Blocking Second Lock**

Traditional lock ordering (always left before right) can still deadlock if multiple threads hold conflicting GPUs. However, `pthread_mutex_trylock()` changes the game:

**Lock Semantics:**
- `pthread_mutex_lock()`: Block if mutex is held; wait indefinitely
- `pthread_mutex_trylock()`: Return immediately; 0 if acquired, non-zero if busy

**Deadlock Prevention Mechanism:**

Deadlock requires:
1. Mutual exclusion (mutex is held exclusively)
2. Hold and wait (thread holds resource and waits for another)
3. No preemption (cannot forcibly release a held mutex)
4. **Circular wait (threads in a cycle, each waiting for the next)** ← **BROKEN HERE**

Our algorithm breaks condition 4 because:
- The first lock is acquired blocking (threads wait)
- The second lock is acquired **non-blocking** (threads do not wait)
- If the second lock is unavailable, the thread **immediately releases the first** and retries
- This prevents a thread from holding a resource while waiting for another indefinitely

**Why No Deadlock:**

Suppose two researchers reach a potential conflict:
- Researcher A holds GPU X and tries to acquire GPU Y
- Researcher B holds GPU Y and tries to acquire GPU X

With our algorithm:
- **If A goes first:** A acquires GPU X (blocking), then tries GPU Y (non-blocking). If Y is held by B, A releases X and retries. No deadlock because A didn't hold both GPUs simultaneously.
- **B can then acquire GPU Y (blocking), then GPU X (non-blocking).** Since A released X, B succeeds and trains.

The non-blocking second lock **guarantees forward progress** because no thread remains holding one resource while blocked on another.

### 4.3 Edge Cases

#### Edge Case 1: Researcher 2's Preference Change

**Scenario:**
- Researchers 0, 1, 3, 4 follow: left GPU → right GPU (if available)
- Researcher 2 follows: right GPU → left GPU

Example:
- Researcher 1 holds GPU 1, waits for GPU 2
- Researcher 2 holds GPU 3, tries GPU 2 first (but it's held by Researcher 1)
- Researcher 2's `trylock` on GPU 2 fails, so it releases GPU 3 and retries
- Now Researcher 2 will eventually succeed because the non-blocking semantics prevent it from deadlocking

**No deadlock because:** Researcher 2 never waits indefinitely; it backs off and retries, giving other threads a chance to release GPUs.

#### Edge Case 2: Thundering Herd on One GPU

**Scenario:** All 5 researchers simultaneously want GPU 0 as their first GPU.

- All 5 call `pthread_mutex_lock(&gpu_mutexes[0])`
- The mutex queue orders them. Researcher A wins and acquires GPU 0.
- A tries its second GPU (non-blocking); if available, it succeeds. If not, A releases GPU 0.
- Researcher B then acquires GPU 0 and tries its second GPU.

**Result:** Despite high contention, no deadlock occurs because threads explicitly release the first GPU if the second is unavailable.

#### Edge Case 3: Retry Fairness

**Scenario:** Researcher X fails to acquire its second GPU 10 times in a row.

With `usleep(35000)`, the retry loop has:
- A brief backoff delay (to reduce lock contention)
- No lock priority/order preservation

**Potential fairness issue:** A researcher might retry more times than others before succeeding. However, this is acceptable because:
- Eventually, other researchers will release GPUs, making the second GPU available
- The exponential backoff (35000 microseconds) ensures threads don't busy-wait
- No starvation occurs in practice with only 5 researchers

### 4.4 Why This Approach is Practical

1. **Simplicity:** Easy to understand and implement. One researcher gets a different preference.
2. **Reduced Contention:** By having one researcher prefer the right GPU, the contention pattern changes, reducing the chance of circular conflicts.
3. **Non-Blocking Semantics:** The `trylock` approach is common in real systems (e.g., database locking, Java's `Lock.tryLock()`).
4. **Debuggability:** The retry loop with sleep is easy to trace in logs.

---

## 5. Comparison of All Four Scenarios

| Aspect | Scenario 1: n-1 Room | Scenario 2: Availability Check | Scenario 3: Asymmetric | Scenario 4: Right-Preference |
|--------|-------|-------|-------|-------|
| **Mechanism** | Semaphore limiting | Atomic check-reserve | Different lock order per parity | Non-blocking second lock + retry |
| **Lock Count** | 5 + 1 semaphore | 1 mutex + 1 semaphore | 5 mutexes | 5 mutexes |
| **Acquisition Model** | Serial room entry | Atomic boolean array | Ordered pair acquisition | Blocking 1st, non-blocking 2nd |
| **Deadlock Prevention** | Room limit guarantees space | All-or-nothing atomicity | Asymmetry breaks circle | No indefinite hold-and-wait |
| **Fairness** | Good (queue-based) | Moderate (looping check) | Good (deterministic order) | Fair if retry backoff is used |
| **Scalability** | Limited by room size | O(busy wait) checking | Excellent (linear) | Good (non-blocking) |

---

## 6. Execution Output Explanation

When the program runs (with scenario ID 0), all four scenarios execute in sequence:

```
=== Scenario 1: Maximum n-1 Researchers ===
[Researcher 1] Analyzing data.
[Researcher 1] Entered limited room.
[Researcher 1] Acquired GPUs 1 and 2.
...

=== Scenario 3: Asymmetric Resource Acquisition ===
[Researcher 1] Analyzing data.
[Researcher 1] Acquired GPUs 1 then 2.  ← Acquired in this specific order
...

=== Scenario 4: Right-GPU Preference ===
[Researcher 3] Analyzing data.
[Researcher 3] Acquired GPUs 4 and 3 with preference.  ← Researcher 2 uses different order
...
```

Key observations:
- No "Acquired Deadlock" or timeout messages (would indicate failure)
- All researchers eventually complete all training rounds
- Output order varies due to thread scheduling, but completion is guaranteed

---

## 7. Code Correctness Checklist

- ✅ No missing `pthread_mutex_unlock()` calls (ensures lock release)
- ✅ No circular waits on the same mutexes (ordered or non-blocking)
- ✅ Proper thread creation and joining (`pthread_create`, `pthread_join`)
- ✅ Semaphore and mutex initialization/cleanup
- ✅ Readable console output for verification and video demonstration
- ✅ Configurable training rounds for test/presentation flexibility

---

## 8. Notes for Presentation and Testing

- **Scenario 3** demonstrates a classic textbook solution to Dining Philosophers
- **Scenario 4** shows a more pragmatic, real-world approach used in many concurrent systems
- Both are deadlock-free, but differ in philosophy: symmetry-breaking vs. non-blocking semantics
- The side-by-side comparison helps students understand multiple valid approaches to the same problem
