#define _GNU_SOURCE // For `CPU_*` fns. Keep above header files.

#include <pthread.h> // For pthread fns
#include <sched.h>   // For `CPU_*` fns
#include <stdio.h>   // For printf()
#include <stdlib.h>  // For malloc() and free()
#include <unistd.h>  // For sleep()

#define NUM_THREADS 2

typedef struct thread_data {
    int thrd_num;
} thread_data_t;

void *thread_fn(void *arg) {
    thread_data_t *thrd_data = (thread_data_t *)arg;
    int *ret_val = (int *)malloc(sizeof(int));
    *ret_val = thrd_data->thrd_num;

    printf("In thread: Executing thread no. %d...\n", thrd_data->thrd_num);
    sleep(thrd_data->thrd_num * 100 + 100);

    pthread_exit((void *)ret_val);
}

int main() {
    pthread_t thread[NUM_THREADS];
    pthread_attr_t *thrd_attr[NUM_THREADS];
    cpu_set_t *cpuset[NUM_THREADS];
    thread_data_t *thrd_data[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        // Thread attributes
        thrd_attr[i] = (pthread_attr_t *)malloc(sizeof(pthread_attr_t));
        pthread_attr_init(thrd_attr[i]);

        // Pinnning thread to CPU
        cpuset[i] = (cpu_set_t *)malloc(sizeof(cpu_set_t));
        CPU_ZERO(cpuset[i]);
        CPU_SET(i + 3, cpuset[i]);
        pthread_attr_setaffinity_np(thrd_attr[i], sizeof(cpu_set_t), cpuset[i]);

        // Providing data to thread
        thrd_data[i] = (thread_data_t *)malloc(sizeof(thread_data_t));
        thrd_data[i]->thrd_num = i;

        // Creating thread
        int ret_val = pthread_create(&thread[i], thrd_attr[i], thread_fn,
                                     (void *)thrd_data[i]);

        if (ret_val != 0) {
            printf("Could not create thread no. %d...\n", i);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        void *ret_val = NULL;

        // Waiting for thread to complete and return a value from the thread fn
        pthread_join(thread[i], &ret_val);

        printf("In main: Thread no. %d completed!\n", *(int *)ret_val);

        // Cleanup
        pthread_attr_destroy(thrd_attr[i]);
        free(ret_val);
        free(cpuset[i]);
        free(thrd_attr[i]);
        free(thrd_data[i]);
    }

    return 0;
}
