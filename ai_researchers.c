#define _DEFAULT_SOURCE

/*
 * Team ownership:
 * - Team Member 2: owns the AI Researchers core architecture, maximum n-1
 *   solution, and GPU availability checking.
 * - Team Member 3: owns asymmetric acquisition, right-GPU preference,
 *   output validation, and scenario coordination.
 * - Team Member 1: reviews shared formatting, integration, and final build flow.
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define RESEARCHER_COUNT 5
#define GPU_COUNT 5
/* Increase training rounds to generate clearer demo output for tests/presentation. */
#define TRAINING_ROUNDS 3

enum
{
    SCENARIO_MAX_N_MINUS_1 = 1,
    SCENARIO_GPU_CHECK = 2,
    SCENARIO_ASYMMETRIC = 3,
    SCENARIO_RIGHT_PREF = 4
};

typedef struct
{
    int researcher_id;
    int scenario_id;
} researcher_args_t;

static pthread_mutex_t gpu_mutexes[GPU_COUNT];
static pthread_mutex_t availability_mutex = PTHREAD_MUTEX_INITIALIZER;
static short gpu_available[GPU_COUNT] = {1, 1, 1, 1, 1};
static sem_t room_limit;

static int left_gpu_of(int researcher_id)
{
    return researcher_id;
}

static int right_gpu_of(int researcher_id)
{
    return (researcher_id + 1) % GPU_COUNT;
}

static void simulate_thinking(int researcher_id)
{
    printf("[Researcher %d] Analyzing data.\n", researcher_id + 1);
    usleep(70000 + researcher_id * 15000);
}

static void simulate_training(int researcher_id, const char *label)
{
    printf("[Researcher %d] Training models using %s.\n", researcher_id + 1, label);
    usleep(100000 + researcher_id * 12000);
    printf("[Researcher %d] Training finished.\n", researcher_id + 1);
}

static void reset_gpu_state(void)
{
    for (int iter_a = 0; iter_a < GPU_COUNT; ++iter_a)
    {
        gpu_available[iter_a] = 1;
    }
}

static void acquire_two_mutexes(int first_gpu, int second_gpu)
{
    pthread_mutex_lock(&gpu_mutexes[first_gpu]);
    pthread_mutex_lock(&gpu_mutexes[second_gpu]);
}

static void release_two_mutexes(int first_gpu, int second_gpu)
{
    pthread_mutex_unlock(&gpu_mutexes[second_gpu]);
    pthread_mutex_unlock(&gpu_mutexes[first_gpu]);
}

static void *max_n_minus_1_worker(void *arg)
{
    researcher_args_t *info = (researcher_args_t *)arg;
    int researcher_id = info->researcher_id;
    int left_gpu = left_gpu_of(researcher_id);
    int right_gpu = right_gpu_of(researcher_id);

    /* Team Member 2: deadlock prevention with the n-1 room limit. */
    for (int iter_a = 0; iter_a < TRAINING_ROUNDS; ++iter_a)
    {
        simulate_thinking(researcher_id);

        sem_wait(&room_limit);
        printf("[Researcher %d] Entered limited room.\n", researcher_id + 1);

        acquire_two_mutexes(left_gpu, right_gpu);
        printf("[Researcher %d] Acquired GPUs %d and %d.\n", researcher_id + 1, left_gpu + 1, right_gpu + 1);

        simulate_training(researcher_id, "two locked GPUs");

        release_two_mutexes(left_gpu, right_gpu);
        sem_post(&room_limit);
        printf("[Researcher %d] Left limited room.\n", researcher_id + 1);
    }

    return NULL;
}

static void *availability_check_worker(void *arg)
{
    researcher_args_t *info = (researcher_args_t *)arg;
    int researcher_id = info->researcher_id;
    int left_gpu = left_gpu_of(researcher_id);
    int right_gpu = right_gpu_of(researcher_id);

    /* Team Member 2: atomic GPU availability check and reserve step. */
    for (int iter_a = 0; iter_a < TRAINING_ROUNDS; ++iter_a)
    {
        simulate_thinking(researcher_id);

        for (;;)
        {
            short cond_flag = 0;

            pthread_mutex_lock(&availability_mutex);
            if (gpu_available[left_gpu] && gpu_available[right_gpu])
            {
                cond_flag = 1;
                gpu_available[left_gpu] = 0;
                gpu_available[right_gpu] = 0;
            }
            pthread_mutex_unlock(&availability_mutex);

            if (cond_flag)
            {
                printf("[Researcher %d] Reserved GPUs %d and %d.\n", researcher_id + 1, left_gpu + 1, right_gpu + 1);
                simulate_training(researcher_id, "reserved GPUs");

                pthread_mutex_lock(&availability_mutex);
                gpu_available[left_gpu] = 1;
                gpu_available[right_gpu] = 1;
                pthread_mutex_unlock(&availability_mutex);
                break;
            }

            usleep(40000);
        }
    }

    return NULL;
}

static void *asymmetric_worker(void *arg)
{
    researcher_args_t *info = (researcher_args_t *)arg;
    int researcher_id = info->researcher_id;
    int left_gpu = left_gpu_of(researcher_id);
    int right_gpu = right_gpu_of(researcher_id);
    int first_gpu = (researcher_id % 2 == 0) ? right_gpu : left_gpu;
    int second_gpu = (researcher_id % 2 == 0) ? left_gpu : right_gpu;

    /* Team Member 3: asymmetric resource acquisition order. */
    for (int iter_a = 0; iter_a < TRAINING_ROUNDS; ++iter_a)
    {
        simulate_thinking(researcher_id);

        acquire_two_mutexes(first_gpu, second_gpu);
        printf("[Researcher %d] Acquired GPUs %d then %d.\n", researcher_id + 1, first_gpu + 1, second_gpu + 1);

        simulate_training(researcher_id, "asymmetric order");

        release_two_mutexes(first_gpu, second_gpu);
    }

    return NULL;
}

static void *right_preference_worker(void *arg)
{
    researcher_args_t *info = (researcher_args_t *)arg;
    int researcher_id = info->researcher_id;
    int left_gpu = left_gpu_of(researcher_id);
    int right_gpu = right_gpu_of(researcher_id);
    int special_researcher_id = 2;

    /* Team Member 3: one researcher prefers the right GPU first. */
    for (int iter_a = 0; iter_a < TRAINING_ROUNDS; ++iter_a)
    {
        int first_gpu = left_gpu;
        int second_gpu = right_gpu;

        if (researcher_id == special_researcher_id)
        {
            first_gpu = right_gpu;
            second_gpu = left_gpu;
        }

        simulate_thinking(researcher_id);

        for (;;)
        {
            pthread_mutex_lock(&gpu_mutexes[first_gpu]);
            if (pthread_mutex_trylock(&gpu_mutexes[second_gpu]) == 0)
            {
                printf("[Researcher %d] Acquired GPUs %d and %d with preference.\n", researcher_id + 1, first_gpu + 1, second_gpu + 1);
                simulate_training(researcher_id, "preferred-order mutexes");
                pthread_mutex_unlock(&gpu_mutexes[second_gpu]);
                pthread_mutex_unlock(&gpu_mutexes[first_gpu]);
                break;
            }

            pthread_mutex_unlock(&gpu_mutexes[first_gpu]);
            usleep(35000);
        }
    }

    return NULL;
}

static void init_mutexes(void)
{
    for (int iter_a = 0; iter_a < GPU_COUNT; ++iter_a)
    {
        pthread_mutex_init(&gpu_mutexes[iter_a], NULL);
    }
}

static void destroy_mutexes(void)
{
    for (int iter_a = 0; iter_a < GPU_COUNT; ++iter_a)
    {
        pthread_mutex_destroy(&gpu_mutexes[iter_a]);
    }
}

static void run_scenario(int scenario_id)
{
    pthread_t threads[RESEARCHER_COUNT];
    researcher_args_t args[RESEARCHER_COUNT];

    reset_gpu_state();

    /* Team Member 2 and Team Member 3: scenario dispatcher. */
    if (scenario_id == SCENARIO_MAX_N_MINUS_1)
    {
        printf("=== Scenario 1: Maximum n-1 Researchers ===\n");
        sem_init(&room_limit, 0, RESEARCHER_COUNT - 1);
    }
    else if (scenario_id == SCENARIO_GPU_CHECK)
    {
        printf("=== Scenario 2: GPU Availability Check ===\n");
    }
    else if (scenario_id == SCENARIO_ASYMMETRIC)
    {
        printf("=== Scenario 3: Asymmetric Resource Acquisition ===\n");
    }
    else if (scenario_id == SCENARIO_RIGHT_PREF)
    {
        printf("=== Scenario 4: Right-GPU Preference ===\n");
    }

    for (int iter_a = 0; iter_a < RESEARCHER_COUNT; ++iter_a)
    {
        args[iter_a].researcher_id = iter_a;
        args[iter_a].scenario_id = scenario_id;

        if (scenario_id == SCENARIO_MAX_N_MINUS_1)
        {
            pthread_create(&threads[iter_a], NULL, max_n_minus_1_worker, &args[iter_a]);
        }
        else if (scenario_id == SCENARIO_GPU_CHECK)
        {
            pthread_create(&threads[iter_a], NULL, availability_check_worker, &args[iter_a]);
        }
        else if (scenario_id == SCENARIO_ASYMMETRIC)
        {
            pthread_create(&threads[iter_a], NULL, asymmetric_worker, &args[iter_a]);
        }
        else
        {
            pthread_create(&threads[iter_a], NULL, right_preference_worker, &args[iter_a]);
        }
    }

    for (int iter_b = 0; iter_b < RESEARCHER_COUNT; ++iter_b)
    {
        pthread_join(threads[iter_b], NULL);
    }

    if (scenario_id == SCENARIO_MAX_N_MINUS_1)
    {
        sem_destroy(&room_limit);
    }
}

static int read_scenario_id(int argc, char *argv[])
{
    if (argc > 1)
    {
        int scenario_id = atoi(argv[1]);

        if (scenario_id >= 0 && scenario_id <= 4)
        {
            return scenario_id;
        }
    }

    int scenario_id = 0;
    int scanned_count = scanf("%d", &scenario_id);

    if (scanned_count != 1 || scenario_id < 0 || scenario_id > 4)
    {
        scenario_id = 0;
    }

    return scenario_id;
}

int main(int argc, char *argv[])
{
    int scenario_id = read_scenario_id(argc, argv);

    init_mutexes();

    if (scenario_id == 0)
    {
        for (int iter_a = SCENARIO_MAX_N_MINUS_1; iter_a <= SCENARIO_RIGHT_PREF; ++iter_a)
        {
            run_scenario(iter_a);
            printf("\n");
        }
    }
    else
    {
        run_scenario(scenario_id);
    }

    destroy_mutexes();
    pthread_mutex_destroy(&availability_mutex);

    return 0;
}
