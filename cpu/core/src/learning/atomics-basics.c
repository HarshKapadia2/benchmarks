#define _GNU_SOURCE // For `CPU_*` fns. Keep above header files.

#include <pthread.h>   // For pthread fns
#include <sched.h>     // For `CPU_*` fns
#include <stdatomic.h> // For `atomic*` fns
#include <stdio.h>     // For printf()
#include <stdlib.h>    // For malloc(), free(), EXIT_SUCCESS and EXIT_FAILURE
// #include <unistd.h>    // For sleep()

#define NUM_THREADS 2

/* atomic_int flag = 0; */
int ctr = 0;
atomic_int a_ctr = 0;

typedef struct thread_data {
    int thrd_num;
} thread_data_t;

void *thread_0_fn(void *arg);
void *thread_1_fn(void *arg);

int main() {
    pthread_t thread[NUM_THREADS];
    pthread_attr_t thrd_attr[NUM_THREADS];
    cpu_set_t cpuset[NUM_THREADS];
    thread_data_t thrd_data[NUM_THREADS];

    //
    //
    // Thread no. 0
    //
    //

    // Thread attributes
    pthread_attr_init(&thrd_attr[0]);

    // Pinnning thread to CPU
    CPU_ZERO(&cpuset[0]);
    CPU_SET(3, &cpuset[0]);
    int ret_val = pthread_attr_setaffinity_np(&thrd_attr[0], sizeof(cpu_set_t),
                                              &cpuset[0]);

    if (ret_val != 0) {
        printf("Could not get CPU affinity for thread no. 0...\n");
        return EXIT_FAILURE;
    }

    // Providing data to thread
    thrd_data[0].thrd_num = 0;

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

    // Thread attributes
    pthread_attr_init(&thrd_attr[1]);

    // Pinnning thread to CPU
    CPU_ZERO(&cpuset[1]);
    CPU_SET(4, &cpuset[1]);
    ret_val = pthread_attr_setaffinity_np(&thrd_attr[1], sizeof(cpu_set_t),
                                          &cpuset[1]);

    if (ret_val != 0) {
        printf("Could not get CPU affinity for thread no. 1...\n");
        return EXIT_FAILURE;
    }

    // Providing data to thread
    thrd_data[1].thrd_num = 1;

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

    printf("Current value of 'ctr': %d\n", ctr);
    printf("Current value of 'a_ctr': %d\n", atomic_load(&a_ctr));

    return EXIT_SUCCESS;
}

void *thread_0_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    int *ret_val = (int *)malloc(sizeof(int));
    *ret_val = thrd_data->thrd_num;

    printf("In thread no. 0.\n");

    /* while (atomic_load(&flag) == 0) */
    /*     ; */

    for (int i = 0; i < 20000000; i++) {
        ctr++;
        atomic_fetch_add(&a_ctr, 1);
    }

    printf("Finished executing thread no. 0.\n");

    pthread_exit((void *)ret_val);
}

void *thread_1_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    int *ret_val = (int *)malloc(sizeof(int));
    *ret_val = thrd_data->thrd_num;

    printf("In thread no. 1.\n");

    /* sleep(5); */

    /* atomic_store(&flag, 1); */

    /* sleep(5); */

    for (int i = 0; i < 20000000; i++) {
        ctr++;
        atomic_fetch_add(&a_ctr, 1);
    }

    printf("Finished executing thread no. 1.\n");

    pthread_exit((void *)ret_val);
}
