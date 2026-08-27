#define _GNU_SOURCE // For `CPU_*` fns. Keep above header files.

#include <pthread.h>   // For pthread fns
#include <sched.h>     // For `CPU_*` fns and sched_getcpu()
#include <stdalign.h>  // For alignas()
#include <stdatomic.h> // For `atomic*` fns
#include <stdint.h>    // For 'uint64_t' type
#include <stdio.h>     // For printf() and file-related fns
#include <stdlib.h>    // For malloc(), free(), EXIT_SUCCESS and EXIT_FAILURE
#include <time.h>      // For clock_gettime() and difftime()

#define NUM_THREADS 2
#define NUM_UPDATES 1000000000ULL
#define CACHE_LINE_SIZE_BYTES 64
/* #define NUM_PHYSICAL_CPU_CORES 64 */
#define NUM_PHYSICAL_CPU_CORES 16

typedef struct thread_data {
    int thrd_num;
    int cpu_num;
} thread_data_t;

typedef struct counter {
    volatile uint64_t counter_0; // Volatile to prevent the compiler from
                                 // optimizing the loop away
    char padding[CACHE_LINE_SIZE_BYTES - (sizeof(uint64_t))]; // Ensures that
                                                              // counter_1 is on
                                                              // the next
                                                              // cacheline
    volatile uint64_t counter_1;
} counter_t;

int create_thread(int thrd_num, int cpu_num, void *(*thrd_fn)(void *arg));
void *thrd_0_fn(void *arg);
void *thrd_1_fn(void *arg);
void print_latency_arr();
void initialize_latency_arr();
void generate_latency_csv();

pthread_t thread[NUM_THREADS];
pthread_attr_t thrd_attr[NUM_THREADS];
cpu_set_t cpuset[NUM_THREADS];
thread_data_t thrd_data[NUM_THREADS];
pthread_barrier_t start_barrier;
pthread_barrier_t finish_barrier;
atomic_int is_thread_fail;
double update_latency_ns[NUM_PHYSICAL_CPU_CORES][NUM_PHYSICAL_CPU_CORES];
alignas(CACHE_LINE_SIZE_BYTES) counter_t ctr;

int main() {
    initialize_latency_arr();

    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        for (int j = 0; j < NUM_PHYSICAL_CPU_CORES; j++) {
            //
            // Don't run the test for own core
            //
            if (i == j) {
                continue;
            }

            //
            // Declarations and initializations
            //
            struct timespec time_start, time_end;
            ctr.counter_0 = 0;
            ctr.counter_1 = 0;
            atomic_store(&is_thread_fail, 0);
            pthread_barrier_init(&start_barrier, NULL, NUM_THREADS + 1);
            pthread_barrier_init(&finish_barrier, NULL, NUM_THREADS + 1);

            //
            // Thread no. 0
            //
            int ret_val = create_thread(0, i, thrd_0_fn);

            if (ret_val != 0) {
                printf("Could not create thread no. 0...\n");
                return ret_val;
            }

            //
            // Thread no. 1
            //
            ret_val = create_thread(1, j, thrd_1_fn);

            if (ret_val != 0) {
                // TODO: Cleanup thread 0 and barrier before exiting

                printf("Could not create thread no. 1...\n");
                return ret_val;
            }

            //
            // Test time measurement
            //
            pthread_barrier_wait(&start_barrier); // Wait for both threads to
                                                  // start and reach the point
                                                  // before looping counters

            clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);

            pthread_barrier_wait(&finish_barrier); // Wait for both threads to
                                                   // finish looping counters

            clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);

            double time_diff_ns =
                (1e9 * difftime(time_end.tv_sec, time_start.tv_sec)) +
                (time_end.tv_nsec - time_start.tv_nsec);
            double avg_update_time_ns = time_diff_ns / (2.0 * NUM_UPDATES);
            update_latency_ns[i][j] = avg_update_time_ns;

            //
            // Wait for thread completion and cleanup
            //
            for (int k = 0; k < NUM_THREADS; k++) {
                void *ret_val_2 = NULL;

                // Waiting for thread to complete
                pthread_join(thread[k], &ret_val_2);

                // TODO: Handle return value for error handling

                // Cleanup
                pthread_attr_destroy(&thrd_attr[k]);
                free(ret_val_2);
            }

            // Cleanup
            pthread_barrier_destroy(&start_barrier);
            pthread_barrier_destroy(&finish_barrier);
        }
    }

    print_latency_arr();
    generate_latency_csv();

    return EXIT_SUCCESS;
}

int create_thread(int thrd_num, int cpu_num, void *(*thrd_fn)(void *arg)) {
    // Init thread attributes
    pthread_attr_init(&thrd_attr[thrd_num]);

    // Pinnning thread to CPU
    CPU_ZERO(&cpuset[thrd_num]);
    CPU_SET(cpu_num, &cpuset[thrd_num]);
    int ret_val = pthread_attr_setaffinity_np(
        &thrd_attr[thrd_num], sizeof(cpu_set_t), &cpuset[thrd_num]);

    if (ret_val != 0) {
        printf("Could not get CPU affinity for thread no. %d...\n", thrd_num);

        pthread_attr_destroy(&thrd_attr[thrd_num]);

        return ret_val;
    }

    // Providing data to thread
    thrd_data[thrd_num].thrd_num = thrd_num;
    thrd_data[thrd_num].cpu_num = cpu_num;

    // Creating thread
    ret_val = pthread_create(&thread[thrd_num], &thrd_attr[thrd_num], thrd_fn,
                             (void *)&thrd_data[thrd_num]);

    if (ret_val != 0) {
        printf("Could not create thread no. %d...\n", thrd_num);

        pthread_attr_destroy(&thrd_attr[thrd_num]);

        return ret_val;
    }

    return ret_val;
}

void *thrd_0_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    int *ret_val = (int *)malloc(sizeof(int));
    *ret_val = 0;
    int current_cpu_num = sched_getcpu();
    volatile uint64_t temp __attribute__((unused)) = 0; // Volatile to prevent
                                                        // the compiler from
                                                        // optimizing the loop
                                                        // away

    if (current_cpu_num != thrd_data->cpu_num) {
        printf("ERROR: Thread no. 0 running on CPU %d instead of %d.\n",
               current_cpu_num, thrd_data->cpu_num);

        *ret_val = -1;

        atomic_store(&is_thread_fail, 1);
    }

    // Wait here and start executing once both threads have reached their
    // barriers. This helps in synchronising the start of the looping, so that
    // thread no. 1 doesn't start before thread no. 0
    pthread_barrier_wait(&start_barrier);

    if (atomic_load(&is_thread_fail) == 1) {
        pthread_exit(ret_val);
    }

    for (uint64_t i = 0; i < NUM_UPDATES; i++) {
        ctr.counter_0++;
    }

    pthread_barrier_wait(&finish_barrier); // Indicate that looping is done

    temp = ctr.counter_0;

    pthread_exit(ret_val);
}

void *thrd_1_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    int *ret_val = (int *)malloc(sizeof(int));
    *ret_val = 0;
    int current_cpu_num = sched_getcpu();
    volatile uint64_t temp __attribute__((unused)) = 0; // Volatile to prevent
                                                        // the compiler from
                                                        // optimizing the loop
                                                        // away

    if (current_cpu_num != thrd_data->cpu_num) {
        printf("ERROR: Thread no. 1 running on CPU %d instead of %d.\n",
               current_cpu_num, thrd_data->cpu_num);

        *ret_val = -1;

        atomic_store(&is_thread_fail, 1);
    }

    // Wait here and start executing once both threads have reached their
    // barriers. This helps in synchronising the start of the looping, so that
    // thread no. 1 doesn't start before thread no. 0
    pthread_barrier_wait(&start_barrier);

    if (atomic_load(&is_thread_fail) == 1) {
        pthread_exit((void *)ret_val);
    }

    for (uint64_t i = 0; i < NUM_UPDATES; i++) {
        ctr.counter_1++;
    }

    pthread_barrier_wait(&finish_barrier); // Indicate that looping is done

    temp = ctr.counter_1;

    pthread_exit((void *)ret_val);
}

void print_latency_arr() {
    printf("\t");

    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        if (i < 10) {
            printf("%d   ", i);
        } else if (i < 100) {
            printf("%d  ", i);
        } else {
            printf("%d ", i);
        }
    }

    printf("\n\n");

    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        printf("%d\t", i);

        for (int j = 0; j < NUM_PHYSICAL_CPU_CORES; j++) {
            printf("%.0f ", update_latency_ns[i][j]);
        }

        printf("\n");
    }
}

void initialize_latency_arr() {
    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        for (int j = 0; j < NUM_PHYSICAL_CPU_CORES; j++) {
            update_latency_ns[i][j] = -1;
        }
    }
}

void generate_latency_csv() {
    FILE *file_ptr = fopen("not-false-sharing-update-latency.csv", "w");

    if (file_ptr == NULL) {
        printf("Could not open file to print CSV file.");
        return;
    }

    fprintf(file_ptr, "src_core,dst_core,latency_ns\n");

    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        for (int j = 0; j < NUM_PHYSICAL_CPU_CORES; j++) {
            fprintf(file_ptr, "%d,%d,%.0f\n", i, j, update_latency_ns[i][j]);
        }
    }

    fclose(file_ptr);
}
