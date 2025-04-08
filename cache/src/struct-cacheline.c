#include <sodium.h> // For generating random integers (needs external packages)
#include <stdio.h>  // For printf
#include <stdlib.h> // For malloc, free
#include <time.h>   // For clock_gettime

typedef struct struct_opti {
    struct struct_opti *nxt_struct;
    int foo;
    int arr[1024];
} opt_struct;

typedef struct struct_unopti {
    struct struct_unopti *nxt_struct;
    int arr[1024];
    int foo;
} unopt_struct;

opt_struct *generate_random_opt_chase(int num_struct);
unopt_struct *generate_random_unopt_chase(int num_struct);

int main(void) {
    printf("Struct cacheline benchmark.\n\n");

    struct timespec time_start, time_end;
    int num_struct = 1000000;

    opt_struct *arr_opt = generate_random_opt_chase(num_struct);

    if (arr_opt == NULL) {
        printf("Error: Could not generate optimized struct chase.\n");
        return -1;
    }

    opt_struct next_opt_struct =
        arr_opt[0]; // What was stored at all arr positions was an int **
                    // typecasted to an int *, so this is like returning the
                    // pointer to its original form

    volatile int temp __attribute__((unused)) =
        -1; // Volatile to prevent the compiler from
            // optimizing loop away

    clock_gettime(CLOCK_MONOTONIC, &time_start);

    for (int i = 0; i < num_struct; i++) {
        // Going to the next element
        temp = next_opt_struct.foo;
        next_opt_struct = *(next_opt_struct.nxt_struct);
    }

    clock_gettime(CLOCK_MONOTONIC, &time_end);

    temp = next_opt_struct.foo; // So that the compiler does not optimize the
    // loop away

    double time_diff_ns = (1e9 * difftime(time_end.tv_sec, time_start.tv_sec)) +
                          (double)(time_end.tv_nsec - time_start.tv_nsec);
    double access_time_per_int_ns = time_diff_ns / num_struct;

    printf("Optimized struct int access time: %lf ns\n",
           access_time_per_int_ns);

    // Cleanup
    if (arr_opt != NULL) {
        free(arr_opt);
    }

    unopt_struct *arr_unopt = generate_random_unopt_chase(num_struct);

    if (arr_unopt == NULL) {
        printf("Error: Could not generate unoptimized struct chase.\n");
        return -1;
    }

    unopt_struct next_unopt_struct =
        arr_unopt[0]; // What was stored at all arr positions was an int **
                      // typecasted to an int *, so this is like returning the
                      // pointer to its original form

    // Reset values
    temp = -1;
    access_time_per_int_ns = -1;

    clock_gettime(CLOCK_MONOTONIC, &time_start);

    for (int i = 0; i < num_struct; i++) {
        // Going to the next element
        temp = next_unopt_struct.foo;
        next_unopt_struct = *(next_unopt_struct.nxt_struct);
    }

    clock_gettime(CLOCK_MONOTONIC, &time_end);

    temp = next_unopt_struct.foo; // So that the compiler does not optimize the
    // loop away

    time_diff_ns = (1e9 * difftime(time_end.tv_sec, time_start.tv_sec)) +
                   (double)(time_end.tv_nsec - time_start.tv_nsec);
    access_time_per_int_ns = time_diff_ns / num_struct;

    printf("Unoptimized struct int access time: %lf ns\n",
           access_time_per_int_ns);

    // Cleanup
    if (arr_unopt != NULL) {
        free(arr_unopt);
    }

    return 0;
}

// Generates an array where each struct contains a pointer to another (randomly
// chosen) struct in the same array, ensuring that a cycle that traverses all
// struct is formed.
//
// NOTE: The following algorithm logic is wholly credited to
// https://github.com/afborchert/pointer-chasing/blob/c827a1e99cb6aa9dea494527799f18cf0c0c8671/random-chase.cpp
// . Credit for helping me understand the algorithm goes to
// https://www.linkedin.com/in/pratikkundnani .
opt_struct *generate_random_opt_chase(int num_struct) {
    if (num_struct <= 0) {
        return NULL;
    }
    if (sodium_init() < 0) {
        return NULL;
    }

    opt_struct *arr = (opt_struct *)malloc(num_struct * sizeof(opt_struct));
    int *idx = (int *)malloc(num_struct * sizeof(int));

    for (int i = 0; i < num_struct; i++) {
        idx[i] = i;
        arr[i].foo = i;
    }

    // Shuffle the index array
    for (int i = 0; i < num_struct; i++) {
        int j = i + randombytes_uniform(num_struct);

        if (j < num_struct && i != j) {
            // Swap idx[i] and idx[j]
            int temp = idx[i];
            idx[i] = idx[j];
            idx[j] = temp;
        }
    }

    for (int i = 1; i < num_struct; i++) {
        arr[idx[i - 1]].nxt_struct = &arr[idx[i]];
    }

    arr[idx[num_struct - 1]].nxt_struct = &arr[idx[0]];

    // Cleanup
    free(idx);

    return arr;
}

// Generates an array where each struct contains a pointer to another (randomly
// chosen) struct in the same array, ensuring that a cycle that traverses all
// struct is formed.
//
// NOTE: The following algorithm logic is wholly credited to
// https://github.com/afborchert/pointer-chasing/blob/c827a1e99cb6aa9dea494527799f18cf0c0c8671/random-chase.cpp
// . Credit for helping me understand the algorithm goes to
// https://www.linkedin.com/in/pratikkundnani .
unopt_struct *generate_random_unopt_chase(int num_struct) {
    if (num_struct <= 0) {
        return NULL;
    }
    if (sodium_init() < 0) {
        return NULL;
    }

    unopt_struct *arr =
        (unopt_struct *)malloc(num_struct * sizeof(unopt_struct));
    int *idx = (int *)malloc(num_struct * sizeof(int));

    for (int i = 0; i < num_struct; i++) {
        idx[i] = i;
        arr[i].foo = i;
    }

    // Shuffle the index array
    for (int i = 0; i < num_struct; i++) {
        int j = i + randombytes_uniform(num_struct);

        if (j < num_struct && i != j) {
            // Swap idx[i] and idx[j]
            int temp = idx[i];
            idx[i] = idx[j];
            idx[j] = temp;
        }
    }

    for (int i = 1; i < num_struct; i++) {
        arr[idx[i - 1]].nxt_struct = &arr[idx[i]];
    }

    arr[idx[num_struct - 1]].nxt_struct = &arr[idx[0]];

    // Cleanup
    free(idx);

    return arr;
}
