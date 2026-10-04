#include <omp.h>
#include <stdio.h>

#define NUM_ELEMENTS 10000000
#define SCALAR 3.0

double a[NUM_ELEMENTS];
double b[NUM_ELEMENTS];
double c[NUM_ELEMENTS];

int main(void) {
    // Test
    /* #pragma omp parallel for schedule(static) */
    /*     for (int i = 0; i < 16; i++) { */
    /*         printf("T%d: i = %d\n", omp_get_thread_num(), i); */
    /*     } */

    // Initialize arrays
#pragma omp parallel for schedule(static)
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        a[i] = b[i] = c[i] = 1.0;
    }

    // Measure operation
    double start_time = omp_get_wtime();

#pragma omp parallel for schedule(static)
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        a[i] = b[i] + SCALAR * c[i];
    }

    double end_time = omp_get_wtime();

    double time_diff_sec = end_time - start_time;

    printf("Time elapsed: %f s\n", time_diff_sec);

    return 0;
}
