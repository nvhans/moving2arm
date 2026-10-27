#include "calculation.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef CALCULATION_TEST_SVE
#include <arm_sve.h>
#endif

static int equivalent(float actual, float expected) {
    if (isnan(expected)) {
        return isnan(actual);
    }
    if (isinf(expected) || expected == 0.0f) {
        return actual == expected && !!signbit(actual) == !!signbit(expected);
    }
    return isfinite(actual) &&
           fabsf(actual - expected) <= 8.0f * FLT_EPSILON * fmaxf(1.0f, fabsf(expected));
}

static int golden_case(void) {
    const float a[] = {1, 4, 9, 16};
    const float b[] = {1, 2, 3, 4};
    const uint32_t expected_greater[] = {0, 1, 1, 1};
    const float expected_add[] = {2, 6, 12, 20};
    const float expected_mul[] = {2, 12, 36, 80};
    const float expected_root[] = {1.414213562f, 3.464101615f, 6, 8.944271910f};
    uint32_t greater[4];
    float add[4], mul[4], root[4];
    const calculation_output out = {greater, add, mul, root};
    calculate(a, b, 4, out);
    for (size_t i = 0; i < 4; ++i) {
        if (greater[i] != expected_greater[i] || add[i] != expected_add[i] ||
            mul[i] != expected_mul[i] || !equivalent(root[i], expected_root[i])) {
            fprintf(stderr, "Golden example differs at element %zu\n", i);
            return 1;
        }
    }
    return 0;
}

static int check_length(size_t n) {
    /* Offset every allocation by one element to exercise unaligned transfers.
     * Input allocations end exactly at n, so ASan catches full-width tail loads.
     * Guard both ends of each output, including the zero-length case.
     */
    float *a_storage = malloc((n + 1) * sizeof(float));
    float *b_storage = malloc((n + 1) * sizeof(float));
    uint32_t *greater = malloc((n + 2) * sizeof(uint32_t));
    float *add = malloc((n + 2) * sizeof(float));
    float *mul = malloc((n + 2) * sizeof(float));
    float *root = malloc((n + 2) * sizeof(float));
    if (!a_storage || !b_storage || !greater || !add || !mul || !root) {
        fprintf(stderr, "Allocation failed\n");
        free(a_storage); free(b_storage); free(greater); free(add); free(mul); free(root);
        return 1;
    }
    float *a = a_storage + 1;
    float *b = b_storage + 1;
    const float values_a[] = {1, 4, 9, 16, -3, 0, -0.0f, NAN, INFINITY, -INFINITY, 2, -2};
    const float values_b[] = {1, 2, 3, 4, -1, -0.0f, -0.0f, 1, 2, 1, NAN, INFINITY};
    for (size_t i = 0; i < n; ++i) {
        a[i] = values_a[i % (sizeof(values_a) / sizeof(values_a[0]))];
        b[i] = values_b[i % (sizeof(values_b) / sizeof(values_b[0]))];
    }
    for (size_t i = 0; i < n + 2; ++i) {
        greater[i] = UINT32_C(0xdeadbeef);
        add[i] = mul[i] = root[i] = -1234567.0f;
    }
    const calculation_output out = {greater + 1, add + 1, mul + 1, root + 1};
    calculate(a, b, n, out);
    int failed = 0;
    for (size_t i = 0; i < n; ++i) {
        /* Independent specification, rather than calling the shared tail. */
        const float sum = a[i] + b[i];
        const float product = sum * b[i];
        if (out.greater[i] != (uint32_t)(a[i] > b[i]) ||
            !equivalent(out.add[i], sum) || !equivalent(out.mul[i], product) ||
            !equivalent(out.root[i], sqrtf(product))) {
            fprintf(stderr, "Length %zu: incorrect result at element %zu\n", n, i);
            failed = 1;
            break;
        }
    }
    if (greater[0] != UINT32_C(0xdeadbeef) || greater[n + 1] != UINT32_C(0xdeadbeef) ||
        add[0] != -1234567.0f || add[n + 1] != -1234567.0f ||
        mul[0] != -1234567.0f || mul[n + 1] != -1234567.0f ||
        root[0] != -1234567.0f || root[n + 1] != -1234567.0f) {
        fprintf(stderr, "Length %zu: wrote outside an output buffer\n", n);
        failed = 1;
    }
    free(a_storage); free(b_storage); free(greater); free(add); free(mul); free(root);
    return failed;
}

int main(void) {
#ifdef CALCULATION_TEST_SVE
    const size_t bits = svcntb() * 8;
    const char *expected_bits = getenv("MOVING2ARM_EXPECT_SVE_BITS");
    if (expected_bits && strtoul(expected_bits, NULL, 10) != bits) {
        fprintf(stderr, "Expected %s-bit SVE vectors, got %zu bits\n", expected_bits, bits);
        return 1;
    }
    printf("SVE vector length: %zu bits\n", bits);
#endif
    if (golden_case()) {
        return 1;
    }
    for (size_t n = 0; n <= 257; ++n) {
        if (check_length(n)) {
            return 1;
        }
    }
    /* The empty-input contract permits null pointers too. */
    const calculation_output empty = {NULL, NULL, NULL, NULL};
    calculate(NULL, NULL, 0, empty);
    puts("Passed golden values, lengths 0..257, masks, tails, unaligned buffers, and special floats.");
    return 0;
}
