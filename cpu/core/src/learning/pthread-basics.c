#include <pthread.h> // For pthread fns
#include <stdio.h>   // For printf()
#include <stdlib.h>  // For malloc()
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
    sleep(thrd_data->thrd_num * 10);

    // Cleanup
    free(thrd_data);

    pthread_exit((void *)ret_val);
}

int main() {
    pthread_t thread[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data_t *thrd_data =
            (thread_data_t *)malloc(sizeof(thread_data_t));
        thrd_data->thrd_num = i;

        int ret_val =
            pthread_create(&thread[i], NULL, thread_fn, (void *)thrd_data);

        if (ret_val != 0) {
            printf("Could not create thread no. %d...\n", i);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        void *ret_val = NULL;

        pthread_join(thread[i], &ret_val);

        printf("In main: Thread no. %d completed!\n", *(int *)ret_val);

        // Cleanup
        free(ret_val);
    }

    return 0;
}
