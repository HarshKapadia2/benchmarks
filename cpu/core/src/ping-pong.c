#define _GNU_SOURCE // For `CPU_*` fns. Keep above header files.

#include <pthread.h>   // For pthread fns
#include <sched.h>     // For `CPU_*` fns and sched_getcpu()
#include <stdalign.h>  // For alignas()
#include <stdatomic.h> // For `atomic*` fns
#include <stdio.h>     // For printf() and file-related fns
#include <stdlib.h>    // For malloc(), free(), EXIT_SUCCESS and EXIT_FAILURE
#include <time.h>      // For clock_gettime() and difftime()

#define NUM_THREADS 2
#define NUM_ROUND_TRIPS 10000000
#define CACHE_LINE_SIZE_BYTES 64
#define NUM_PHYSICAL_CPU_CORES 64

typedef struct thread_data {
    int thrd_num;
    int cpu_num;
} thread_data_t;

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
atomic_int is_thread_fail = 0;
alignas(CACHE_LINE_SIZE_BYTES) atomic_int ping = 0;
alignas(CACHE_LINE_SIZE_BYTES) atomic_int pong = 0;
double core_to_core_latency_ns[NUM_PHYSICAL_CPU_CORES][NUM_PHYSICAL_CPU_CORES];

int main() {
    initialize_latency_arr();

    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        for (int j = 0; j < NUM_PHYSICAL_CPU_CORES; j++) {
            // Don't run the test for own core
            if (i == j) {
                continue;
            }

            // Initializations
            atomic_store(&is_thread_fail, 0);
            atomic_store(&ping, 0);
            atomic_store(&pong, 0);
            pthread_barrier_init(&start_barrier, NULL, NUM_THREADS);

            // Thread no. 0
            int ret_val = create_thread(0, i, thrd_0_fn);

            if (ret_val != 0) {
                printf("Could not create thread no. 0...\n");
                return ret_val;
            }

            // Thread no. 1
            ret_val = create_thread(1, j, thrd_1_fn);

            if (ret_val != 0) {
                // TODO: Cleanup thread 0 and barrier before exiting

                printf("Could not create thread no. 1...\n");
                return ret_val;
            }

            // Wait for thread completion and cleanup
            for (int k = 0; k < NUM_THREADS; k++) {
                void *ret_val_2 = NULL;

                // Waiting for thread to complete and return a value from the
                // thread fn
                pthread_join(thread[k], &ret_val_2);

                if (ret_val_2 != NULL && *(double *)ret_val_2 >= 0) {
                    core_to_core_latency_ns[i][j] = *(double *)ret_val_2;
                }

                /* printf("In main: Thread no. %f completed!\n", */
                /*        *(double *)ret_val_2); */

                // Cleanup
                pthread_attr_destroy(&thrd_attr[k]);
                free(ret_val_2);
            }

            // Cleanup
            pthread_barrier_destroy(&start_barrier);
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
    double *ret_val = (double *)malloc(sizeof(double));
    *ret_val = -2;
    int current_cpu_num = sched_getcpu();

    if (current_cpu_num != thrd_data->cpu_num) {
        printf("ERROR: Thread no. 0 running on CPU %d instead of %d.\n",
               current_cpu_num, thrd_data->cpu_num);

        *ret_val = -1;

        atomic_store(&is_thread_fail, 1);
    }

    /* printf("In thread no. 0 on CPU %d.\n", current_cpu_num); */

    // Wait here and start executing once both threads have reached their
    // barriers. This helps in synchronising the start of the looping, so that
    // thread no. 1 doesn't start before thread no. 0
    pthread_barrier_wait(&start_barrier);

    if (atomic_load(&is_thread_fail) == 1) {
        pthread_exit(ret_val);
    }

    for (int i = 0; i < NUM_ROUND_TRIPS; i++) {
        while (atomic_load(&ping) == 0)
            ;

        atomic_store(&ping, 0); // Keep above pong's atomic store to prevent
                                // possible deadlock of ping in the case when T1
                                // makes ping = 1, but T0 sets ping = 0 after
                                // that
        atomic_store(&pong, 1);
    }

    /* printf("Finished executing thread no. 0.\n"); */

    pthread_exit(ret_val);
}

void *thrd_1_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    double *ret_val = (double *)malloc(sizeof(double));
    *ret_val = -1;
    struct timespec time_start, time_end;
    int current_cpu_num = sched_getcpu();

    if (current_cpu_num != thrd_data->cpu_num) {
        printf("ERROR: Thread no. 1 running on CPU %d instead of %d.\n",
               current_cpu_num, thrd_data->cpu_num);

        atomic_store(&is_thread_fail, 1);
    }

    /* printf("In thread no. 1 on CPU %d.\n", current_cpu_num); */

    // Wait here and start executing once both threads have reached their
    // barriers. This helps in synchronising the start of the looping, so that
    // thread no. 1 doesn't start before thread no. 0
    pthread_barrier_wait(&start_barrier);

    if (atomic_load(&is_thread_fail) == 1) {
        pthread_exit((void *)ret_val);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);

    for (int i = 0; i < NUM_ROUND_TRIPS; i++) {
        atomic_store(&ping, 1);

        while (atomic_load(&pong) == 0)
            ;

        atomic_store(&pong, 0);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);

    /* printf("Finished executing thread no. 1.\n"); */

    double time_diff_ns = (1e9 * difftime(time_end.tv_sec, time_start.tv_sec)) +
                          (time_end.tv_nsec - time_start.tv_nsec);
    double avg_round_trip_time_ns = time_diff_ns / NUM_ROUND_TRIPS;
    double avg_core_to_core_latency_ns = avg_round_trip_time_ns / 2.0;
    *ret_val = avg_core_to_core_latency_ns;

    /* printf("Avg. core-to-core latency: %.2f ns\n",
     * avg_core_to_core_latency_ns); */

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
            printf("%.0f ", core_to_core_latency_ns[i][j]);
        }

        printf("\n");
    }
}

void initialize_latency_arr() {
    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        for (int j = 0; j < NUM_PHYSICAL_CPU_CORES; j++) {
            core_to_core_latency_ns[i][j] = -1;
        }
    }
}

void generate_latency_csv() {
    FILE *file_ptr = fopen("core-to-core-latency.csv", "w");

    if (file_ptr == NULL) {
        printf("Could not open file to print CSV file.");
        return;
    }

    fprintf(file_ptr, "src_core,dst_core,latency_ns\n");

    for (int i = 0; i < NUM_PHYSICAL_CPU_CORES; i++) {
        for (int j = 0; j < NUM_PHYSICAL_CPU_CORES; j++) {
            fprintf(file_ptr, "%d,%d,%.0f\n", i, j,
                    core_to_core_latency_ns[i][j]);
        }
    }

    fclose(file_ptr);
}
