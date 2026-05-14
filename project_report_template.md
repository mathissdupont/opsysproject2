# Project Report Template

## Title Page

- Project name
- Group members
- Repository folder name
- YouTube presentation link

## 1. Purpose

Explain the objective of the assignment and the two synchronization problems.

## 2. Scope

Describe the thread model, shared resources, and synchronization primitives.

## 3. Source Code Overview

This project implements two synchronization problems: the Sleeping IT Support (`it_support.c`) and the AI Researchers and GPUs (`ai_researchers.c`).

- `it_support.c`: Producer/consumer simulation with a bounded waiting queue (capacity 5), two specialist (consumer) threads, and multiple customer (producer) threads. Uses a mutex to protect the queue and a semaphore (`waiting_tickets`) for efficient sleep/wake semantics. A second semaphore (`completed_ticket_signal`) is used to coordinate shutdown after accepted tickets complete.

- `ai_researchers.c`: Simulates `RESEARCHER_COUNT` researchers competing for `GPU_COUNT` GPUs. Key components:
	- GPUs are modeled as an array of `pthread_mutex_t` (`gpu_mutexes[]`).
	- Four scenarios are implemented and selectable at runtime:
		1. Maximum n-1 Researchers: Uses a counting semaphore `room_limit` initialized to `RESEARCHER_COUNT - 1` to prevent deadlock (classic dining-philosophers solution variant).
		2. GPU Availability Check: Uses a separate `availability_mutex` and a `gpu_available[]` boolean array to atomically check-and-reserve both GPUs before training; frees them after use.
		3. Asymmetric Resource Acquisition: Odd/even researchers acquire GPUs in opposite orders to break circular wait.
		4. Right-GPU Preference: A special researcher prefers the right GPU first and uses `pthread_mutex_trylock` with retries to avoid blocking.
	- Helper functions `simulate_thinking()` and `simulate_training()` produce readable console output for the video and testing.

Build & run:

```
make
bash test_script.sh   # runs both programs; pass scenario id to `ai_researchers` to run a single scenario
```

Notes for maintainers: the `ai_researchers.c` implementation keeps training rounds configurable via `TRAINING_ROUNDS` to generate demonstrative logs for presentation and testing.

## 4. Code Explanations

Explain each major function and the data flow.

## 5. Execution Screenshots

Insert terminal output screenshots from `test_script.sh` and direct program runs.

## 6. Video Link

Add the unlisted YouTube link here.
