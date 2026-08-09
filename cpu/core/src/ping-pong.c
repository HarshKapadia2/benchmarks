#define _GNU_SOURCE // For `CPU_*` fns. Keep above header files.

#include <pthread.h>   // For pthread fns
#include <sched.h>     // For `CPU_*` fns and sched_getcpu()
#include <stdalign.h>  // For alignas()
#include <stdatomic.h> // For `atomic*` fns
#include <stdio.h>     // For printf()
#include <stdlib.h>    // For malloc(), free(), EXIT_SUCCESS and EXIT_FAILURE
#include <time.h>      // For clock_gettime() and difftime()

#define NUM_THREADS 2
#define NUM_ROUND_TRIPS 10000000
#define CACHE_LINE_SIZE_BYTES 64

typedef struct thread_data {
    int thrd_num;
    int cpu_num;
} thread_data_t;

void *thread_0_fn(void *arg);
void *thread_1_fn(void *arg);

alignas(CACHE_LINE_SIZE_BYTES) atomic_int ping = 0;
alignas(CACHE_LINE_SIZE_BYTES) atomic_int pong = 0;
atomic_int is_thread_fail = 0;
pthread_barrier_t start_barrier;

int main() {
    pthread_t thread[NUM_THREADS];
    pthread_attr_t thrd_attr[NUM_THREADS];
    cpu_set_t cpuset[NUM_THREADS];
    thread_data_t thrd_data[NUM_THREADS];

    // Initialize start barrier to later synchronize ping-pong loop start
    pthread_barrier_init(&start_barrier, NULL, NUM_THREADS);

    //
    //
    // Thread no. 0
    //
    //

    int thrd_0_cpu_num = 3;

    // Thread attributes
    pthread_attr_init(&thrd_attr[0]);

    // Pinnning thread to CPU
    CPU_ZERO(&cpuset[0]);
    CPU_SET(thrd_0_cpu_num, &cpuset[0]);
    int ret_val = pthread_attr_setaffinity_np(&thrd_attr[0], sizeof(cpu_set_t),
                                              &cpuset[0]);

    if (ret_val != 0) {
        printf("Could not get CPU affinity for thread no. 0...\n");
        return EXIT_FAILURE;
    }

    // Providing data to thread
    thrd_data[0].thrd_num = 0;
    thrd_data[0].cpu_num = thrd_0_cpu_num;

    // Creating thread
    ret_val = pthread_create(&thread[0], &thrd_attr[0], thread_0_fn,
                             (void *)&thrd_data[0]);

    if (ret_val != 0) {
        printf("Could not create thread no. 0...\n");
        return EXIT_FAILURE;
    }

    //
    //
    // Thread no. 1
    //
    //

    int thrd_1_cpu_num = 4;

    // Thread attributes
    pthread_attr_init(&thrd_attr[1]);

    // Pinnning thread to CPU
    CPU_ZERO(&cpuset[1]);
    CPU_SET(thrd_1_cpu_num, &cpuset[1]);
    ret_val = pthread_attr_setaffinity_np(&thrd_attr[1], sizeof(cpu_set_t),
                                          &cpuset[1]);

    if (ret_val != 0) {
        printf("Could not get CPU affinity for thread no. 1...\n");
        return EXIT_FAILURE;
    }

    // Providing data to thread
    thrd_data[1].thrd_num = 1;
    thrd_data[1].cpu_num = thrd_1_cpu_num;

    // Creating thread
    ret_val = pthread_create(&thread[1], &thrd_attr[1], thread_1_fn,
                             (void *)&thrd_data[1]);

    if (ret_val != 0) {
        printf("Could not create thread no. 1...\n");
        return EXIT_FAILURE;
    }

    //
    //
    // Wait for thread completion and cleanup
    //
    //

    for (int i = 0; i < NUM_THREADS; i++) {
        void *ret_val_2 = NULL;

        // Waiting for thread to complete and return a value from the thread fn
        pthread_join(thread[i], &ret_val_2);

        printf("In main: Thread no. %d completed!\n", *(int *)ret_val_2);

        // Cleanup
        pthread_attr_destroy(&thrd_attr[i]);
        free(ret_val_2);
    }

    // Cleanup
    pthread_barrier_destroy(&start_barrier);

    return EXIT_SUCCESS;
}

void *thread_0_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    int *ret_val = (int *)malloc(sizeof(int));
    *ret_val = thrd_data->thrd_num;
    int current_cpu_num = sched_getcpu();

    if (current_cpu_num != thrd_data->cpu_num) {
        printf("ERROR: Thread no. 0 running on CPU %d instead of %d.\n",
               current_cpu_num, thrd_data->cpu_num);

        atomic_store(&is_thread_fail, 1);
    }

    printf("In thread no. 0 on CPU %d.\n", current_cpu_num);

    // Wait here and start executing once both threads have reached their
    // barriers. This helps in synchronising the start of the looping, so that
    // thread no. 1 doesn't start before thread no. 0
    pthread_barrier_wait(&start_barrier);

    if (atomic_load(&is_thread_fail) == 1) {
        pthread_exit((void *)ret_val);
    }

    for (int i = 0; i < NUM_ROUND_TRIPS; i++) {
        while (atomic_load(&ping) == 0)
            ;

        atomic_store(&ping, 0); // Keep above to prevent possible deadlock of
                                // ping, if T1 makes ping = 1, but T0 sets ping
                                // = 0 after that.
        atomic_store(&pong, 1);
    }

    printf("Finished executing thread no. 0.\n");

    pthread_exit((void *)ret_val);
}

void *thread_1_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    int *ret_val = (int *)malloc(sizeof(int));
    *ret_val = thrd_data->thrd_num;
    struct timespec time_start, time_end;
    int current_cpu_num = sched_getcpu();

    if (current_cpu_num != thrd_data->cpu_num) {
        printf("ERROR: Thread no. 1 running on CPU %d instead of %d.\n",
               current_cpu_num, thrd_data->cpu_num);

        atomic_store(&is_thread_fail, 1);
    }

    printf("In thread no. 1 on CPU %d.\n", current_cpu_num);

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

    printf("Finished executing thread no. 1.\n");

    double time_diff_ns = (1e9 * difftime(time_end.tv_sec, time_start.tv_sec)) +
                          (time_end.tv_nsec - time_start.tv_nsec);
    double avg_round_trip_time_ns = time_diff_ns / NUM_ROUND_TRIPS;
    double avg_core_to_core_latency_ns = avg_round_trip_time_ns / 2.0;

    printf("Avg. core-to-core latency: %.2f ns\n", avg_core_to_core_latency_ns);

    pthread_exit((void *)ret_val);
}
