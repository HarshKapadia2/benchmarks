#include <math.h>   // For pow
#include <sodium.h> // For generating random integers (needs external packages)
#include <stdio.h>  // For printf
#include <stdlib.h> // For malloc, free
#include <time.h>   // For clock_gettime

int **generate_random_ptr_chase(long arr_size_bytes);

int main() {
    printf("Cache size benchmark.\n\n");
    printf("Array Size\tAccess Time Per Ele. (ns)\n");

    for (int i = 4; i < 30; i++) {
        long arr_size_bytes = (long)pow(2, i);
        int num_itr_per_run = 100000;
        struct timespec time_start, time_end;

        int **arr = generate_random_ptr_chase(arr_size_bytes);

        if (arr == NULL) {
            printf("Warning: Could not generate pointer chase for array size "
                   "%ld.\n",
                   arr_size_bytes);
            continue;
        }

        int **next_element =
            (int **)arr[0]; // What was stored at all arr positions
                            // was an int ** typecasted to an int *, so this is
                            // like returning the pointer to its original form

        int *volatile temp __attribute__((unused)) =
            NULL; // Volatile to prevent the compiler from
                  // optimizing loop away

        clock_gettime(CLOCK_MONOTONIC, &time_start);

        for (int j = 0; j < num_itr_per_run; j++) {
            // Going to the next element
            next_element = (int **)*next_element;
        }

        clock_gettime(CLOCK_MONOTONIC, &time_end);

        temp = *next_element + 1; // So that the compiler does not optimize the
                                  // loop away

        double time_diff_ns =
            (1e9 * difftime(time_end.tv_sec, time_start.tv_sec)) +
            (double)(time_end.tv_nsec - time_start.tv_nsec);
        double access_time_per_ele_ns = time_diff_ns / num_itr_per_run;

        if (i < 10) {
            printf("%ld B\t\t%lf\n", arr_size_bytes, access_time_per_ele_ns);
        } else if (i < 20) {
            int arr_size_kiB = arr_size_bytes / 1024; // 1024 = 2e10

            printf("%d kiB\t\t%lf\n", arr_size_kiB, access_time_per_ele_ns);
        } else {
            int arr_size_MiB = arr_size_bytes / 1048576; // 1048576 = 2e20

            printf("%d MiB\t\t%lf\n", arr_size_MiB, access_time_per_ele_ns);
        }

        // Cleanup
        if (arr != NULL) {
            free(arr);
        }
    }

    return 0;
}

// Generates an array where each element contains a pointer to another (randomly
// chosen) element in the same array, ensuring that a cycle that traverses all
// elements is formed.
//
// NOTE: The following algorithm logic is wholly credited to
// https://github.com/afborchert/pointer-chasing/blob/c827a1e99cb6aa9dea494527799f18cf0c0c8671/random-chase.cpp
// . Credit for helping me understand the algorithm goes to
// https://www.linkedin.com/in/pratikkundnani .
int **generate_random_ptr_chase(long arr_size_bytes) {
    if (arr_size_bytes <= 0) {
        return NULL;
    }
    if (sodium_init() < 0) {
        return NULL;
    }

    int num_ele = (int)(arr_size_bytes / sizeof(int *));
    int **arr = (int **)malloc(num_ele * sizeof(int *));
    int *idx = (int *)malloc(num_ele * sizeof(int));

    for (int i = 0; i < num_ele; i++) {
        idx[i] = i;
    }

    // Shuffle the index array
    for (int i = 0; i < num_ele; i++) {
        int j = i + randombytes_uniform(num_ele);

        if (j < num_ele && i != j) {
            // Swap idx[i] and idx[j]
            int temp = idx[i];
            idx[i] = idx[j];
            idx[j] = temp;
        }
    }

    for (int i = 1; i < num_ele; i++) {
        // LHS = Integer pointer
        // RHS = (Pointer to an integer pointer, i.e., int **) is typecasted to
        // an (integer pointer, i.e., int *)
        arr[idx[i - 1]] = (int *)&arr[idx[i]];
    }

    arr[idx[num_ele - 1]] = (int *)&arr[idx[0]];

    // Cleanup
    free(idx);

    return arr;
}
