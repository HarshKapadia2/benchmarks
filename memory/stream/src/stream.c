#include <omp.h>     // For OpenMP
#include <stdbool.h> // For `bool`, `true`, `false`
#include <stdio.h>   // For printf(), sizeof()
#include <stdlib.h>  // For malloc(), posix_memalign()

#define CACHE_LINE_SIZE_BYTES 64
#define ARRAY_SIZE_BYTES 1073741824 // Array size = 4 * total(last_level_cache)
#define NUM_TEST_RUNS 20
#define NUM_KERNELS 4
#define SCALAR 3.0

enum kernel_codes { COPY = 0, SCALE = 1, ADD = 2, TRIAD = 3 };

const size_t num_elements = ARRAY_SIZE_BYTES / sizeof(double);
const size_t copy_kernel_transfer_bytes = 2 * num_elements * sizeof(double);
const size_t scale_kernel_transfer_bytes = 2 * num_elements * sizeof(double);
const size_t add_kernel_transfer_bytes = 3 * num_elements * sizeof(double);
const size_t triad_kernel_transfer_bytes = 3 * num_elements * sizeof(double);
double *restrict a; // `restrict` tells the compilier that the arrays don't
double *restrict b; // overlap, indicating no pointer aliasing between arrays,
double *restrict c; // enabling more aggressive optimizations
double *test_time_diff_sec[NUM_KERNELS];
int num_threads = 0;

void calculate_bandwidth(void);
bool validate_arrays(void);

int main(void) {
    double start_time_sec = 0;
    double end_time_sec = 0;

    // Memory allocation
    int ret_val = posix_memalign((void **)&a, CACHE_LINE_SIZE_BYTES,
                                 num_elements * sizeof(double));
    ret_val = posix_memalign((void **)&b, CACHE_LINE_SIZE_BYTES,
                             num_elements * sizeof(double));
    ret_val = posix_memalign((void **)&c, CACHE_LINE_SIZE_BYTES,
                             num_elements * sizeof(double));

    for (size_t i = 0; i < NUM_KERNELS; i++) {
        test_time_diff_sec[i] =
            (double *)malloc(NUM_TEST_RUNS * sizeof(double));

        if (test_time_diff_sec[i] == NULL) {
            ret_val = -1;
            break;
        }
    }

    if (ret_val != 0) {
        printf("ERROR: Could not allocate memory for arrays.\n");
        return ret_val;
    }

#pragma omp parallel
    {
#pragma omp single
        num_threads = omp_get_num_threads();

        // Initialize arrays
#pragma omp for schedule(static)
        for (size_t i = 0; i < num_elements; i++) {
            a[i] = 1.0;
            b[i] = 2.0;
            c[i] = 0.0;
        }

        // Run test
        for (size_t i = 0; i < NUM_TEST_RUNS; i++) {
            //
            //
            // Copy kernel
            //
            //
#pragma omp masked filter(0)
            start_time_sec = omp_get_wtime();

#pragma omp barrier

#pragma omp for schedule(static)
            for (size_t j = 0; j < num_elements; j++) {
                c[j] = a[j];
            }

#pragma omp barrier

#pragma omp masked filter(0)
            {
                end_time_sec = omp_get_wtime();
                test_time_diff_sec[COPY][i] = end_time_sec - start_time_sec;
            }

            //
            //
            // Scale kernel
            //
            //
#pragma omp masked filter(0)
            start_time_sec = omp_get_wtime();

#pragma omp barrier

#pragma omp for schedule(static)
            for (size_t j = 0; j < num_elements; j++) {
                b[j] = SCALAR * c[j];
            }

#pragma omp barrier

#pragma omp masked filter(0)
            {
                end_time_sec = omp_get_wtime();
                test_time_diff_sec[SCALE][i] = end_time_sec - start_time_sec;
            }

            //
            //
            // Add kernel
            //
            //
#pragma omp masked filter(0)
            start_time_sec = omp_get_wtime();

#pragma omp barrier

#pragma omp for schedule(static)
            for (size_t j = 0; j < num_elements; j++) {
                c[j] = a[j] + b[j];
            }

#pragma omp barrier

#pragma omp masked filter(0)
            {
                end_time_sec = omp_get_wtime();
                test_time_diff_sec[ADD][i] = end_time_sec - start_time_sec;
            }

            //
            //
            // Triad kernel
            //
            //
#pragma omp masked filter(0)
            start_time_sec = omp_get_wtime();

#pragma omp barrier

#pragma omp for schedule(static)
            for (size_t j = 0; j < num_elements; j++) {
                a[j] = b[j] + (SCALAR * c[j]);
            }

#pragma omp barrier

#pragma omp masked filter(0)
            {
                end_time_sec = omp_get_wtime();
                test_time_diff_sec[TRIAD][i] = end_time_sec - start_time_sec;
            }
        }
    }

    if (validate_arrays() == false) {
        printf("ERROR: Array validation failed.\n");
        return EXIT_FAILURE;
    }

    calculate_bandwidth();

    // Cleanup
    free(a);
    free(b);
    free(c);

    for (size_t i = 0; i < NUM_KERNELS; i++) {
        free(test_time_diff_sec[i]);
    }

    return EXIT_SUCCESS;
}

void calculate_bandwidth() {
    double min_time_diff_sec[NUM_KERNELS];

    // Find min time for each kernel
    for (size_t i = 0; i < NUM_KERNELS; i++) {
        min_time_diff_sec[i] = test_time_diff_sec[i][0];

        for (size_t j = 1; j < NUM_TEST_RUNS; j++) {
            if (test_time_diff_sec[i][j] < min_time_diff_sec[i]) {
                min_time_diff_sec[i] = test_time_diff_sec[i][j];
            }
        }
    }

    // Calculate the memory bandwidth
    const double copy_kernel_bandwidth_gigabytespersec =
        (copy_kernel_transfer_bytes / min_time_diff_sec[COPY]) * 1e-9;
    const double scale_kernel_bandwidth_gigabytespersec =
        (scale_kernel_transfer_bytes / min_time_diff_sec[SCALE]) * 1e-9;
    const double add_kernel_bandwidth_gigabytespersec =
        (add_kernel_transfer_bytes / min_time_diff_sec[ADD]) * 1e-9;
    const double triad_kernel_bandwidth_gigabytespersec =
        (triad_kernel_transfer_bytes / min_time_diff_sec[TRIAD]) * 1e-9;

    printf("%d,%f,%f,%f,%f\n", num_threads,
           copy_kernel_bandwidth_gigabytespersec,
           scale_kernel_bandwidth_gigabytespersec,
           add_kernel_bandwidth_gigabytespersec,
           triad_kernel_bandwidth_gigabytespersec);
}

// The validation is rudimentary and is only done to consume the arrays, so that
// the compliler doesn't optimize them away
bool validate_arrays() {
    double a_val = 1.0;
    double b_val = 2.0;
    double c_val = 0.0;

    for (size_t i = 0; i < NUM_TEST_RUNS; i++) {
        c_val = a_val;                    // Copy kernel
        b_val = SCALAR * c_val;           // Scale kernel
        c_val = a_val + b_val;            // Add kernel
        a_val = b_val + (SCALAR * c_val); // Triad kernel
    }

    for (size_t i = 0; i < num_elements; i++) {
        if (a[i] != a_val || b[i] != b_val || c[i] != c_val) {
            return false;
        }
    }

    return true;
}
