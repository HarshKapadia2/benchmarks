#include <stdio.h>
#include <time.h>

int main() {
    int num_elements = 1000000;
    int stride[] = {1, 8, 16, 32, 64, 128, 256, 512};
    int num_test_itr = sizeof(stride) / sizeof(int);
    int arr[num_elements] __attribute__((unused));

    printf("Cache line size benchmark:\n\n");
    printf("Size of 'int': %ld B\n", sizeof(int));
    printf("Cache line size: 64 B\n\n");
    printf("Page size: 4096 B\n\n");

    for (int j = 0; j < num_test_itr; j++) {
        struct timespec time_start, time_end;

        clock_gettime(CLOCK_MONOTONIC, &time_start);

        for (int i = 0; i < num_elements; i += stride[j]) {
            arr[i] = i * 2;
        }

        clock_gettime(CLOCK_MONOTONIC, &time_end);

        double time_diff_ms =
            (1e3 * difftime(time_end.tv_sec, time_start.tv_sec)) +
            (1e-6 * (double)(time_end.tv_nsec - time_start.tv_nsec));

        printf("Time taken for stride %d elements: %lf ms\n", stride[j],
               time_diff_ms);
    }

    return 0;
}
