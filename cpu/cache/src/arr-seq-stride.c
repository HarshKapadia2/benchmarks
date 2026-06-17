#include <stdio.h>  // For printf
#include <stdlib.h> // For malloc, free
#include <time.h>   // For clock_gettime
#include <unistd.h> // For getpagesize

int *generate_arr(int num_elements);

int main() {
    int num_elements = 20000000;
    int stride[] = {1, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
    int num_test_itr = sizeof(stride) / sizeof(int);
    volatile int temp __attribute__((unused)) = -1;
    int *arr = generate_arr(num_elements);

    if (arr == NULL) {
        printf("Error: Could not generate array.\n ");
        return 1;
    }

    printf("Strided sequential array read scan\n\n");
    printf("Size of 'int': %ld B\n", sizeof(int));
    printf("Cache line size: %ld B\n", sysconf(_SC_LEVEL1_DCACHE_LINESIZE));
    printf("Page size: %d B\n", getpagesize());
    printf("Num elements: %d B\n", num_elements);
    printf("Size of array: %ld B\n\n", num_elements * sizeof(int));
    printf("Num Stride Ele\tStride Size (B)\tNum Accesses\tTotal Time "
           "(ms)\tAccess Time Per Ele (ns)\n");

    for (int j = 0; j < num_test_itr; j++) {
        struct timespec time_start, time_end;
        temp = -1;

        clock_gettime(CLOCK_MONOTONIC, &time_start);

        for (int i = 0; i < num_elements; i += stride[j]) {
            temp += arr[i];
        }

        clock_gettime(CLOCK_MONOTONIC, &time_end);

        temp += 1; // So that the compiler does not optimize the loop away

        double time_diff_ms =
            (1e3 * difftime(time_end.tv_sec, time_start.tv_sec)) +
            (1e-6 * (double)(time_end.tv_nsec - time_start.tv_nsec));
        double num_accesses = (double)num_elements / stride[j];
        double access_time_ns = (time_diff_ms / num_accesses) * 1e6;

        printf("%d\t\t%ld\t\t%.3lf\t%.3lf\t\t%.3lf\n", stride[j],
               stride[j] * sizeof(int), num_accesses, time_diff_ms,
               access_time_ns);
    }

    temp += 1; // So that the compiler does not optimize the loop away

    // Cleanup
    free(arr);

    return 0;
}

int *generate_arr(int num_elements) {
    if (num_elements <= 0) {
        return NULL;
    }

    int *arr = (int *)malloc(num_elements * sizeof(int));

    if (arr == NULL) {
        return NULL;
    }

    for (int i = 0; i < num_elements; i++) {
        arr[i] = i * 2;
    }

    return arr;
}
