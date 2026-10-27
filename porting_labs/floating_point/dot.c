#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

_Static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24, "Exercise requires binary32 floats");
_Static_assert(LDBL_MANT_DIG > FLT_MANT_DIG, "Reference needs more precision than float");

static float separate(float a, float b, float c) {
    /* Force rounding of the product even when contraction is enabled. */
    volatile float product = a * b;
    return product + c;
}

static float expression(float a, float b, float c) {
    return a * b + c; /* This expression may contract, depending on compiler flags. */
}

static float dot_sequential(const float *a, const float *b, size_t n) {
    float sum = 0;
    for (size_t i = 0; i < n; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

static float dot_fused(const float *a, const float *b, size_t n) {
    float sum = 0;
    for (size_t i = 0; i < n; ++i) {
        sum = fmaf(a[i], b[i], sum);
    }
    return sum;
}

static float dot_two_accumulators(const float *a, const float *b, size_t n) {
    float even = 0, odd = 0;
    size_t i = 0;
    for (; n - i >= 2; i += 2) {
        even += a[i] * b[i];
        odd += a[i + 1] * b[i + 1];
    }
    if (i < n) {
        even += a[i] * b[i];
    }
    return even + odd;
}

static long double dot_reference(const float *a, const float *b, size_t n) {
    long double sum = 0;
    for (size_t i = 0; i < n; ++i) {
        sum += (long double)a[i] * (long double)b[i];
    }
    return sum;
}

static int benchmark(void) {
    enum { COUNT = 8192, REPEATS = 256 };
    float *a = malloc(COUNT * sizeof(float));
    float *b = malloc(COUNT * sizeof(float));
    if (!a || !b) {
        free(a); free(b);
        return 1;
    }
    for (size_t i = 0; i < COUNT; ++i) {
        a[i] = (float)((int)(i % 23) - 11) * 0.1f;
        b[i] = (float)((int)(i % 17) - 8) * 0.2f;
    }
    float (*functions[])(const float *, const float *, size_t) = {
        dot_sequential, dot_fused, dot_two_accumulators
    };
    const char *names[] = {"sequential", "fused", "two accumulators"};
    for (size_t method = 0; method < 3; ++method) {
        volatile float sink = 0;
        const clock_t begin = clock();
        for (unsigned repeat = 0; repeat < REPEATS; ++repeat) {
            /* Vary an input to prevent hoisting an invariant dot product. */
            a[0] = (float)(repeat % 13);
            sink = functions[method](a, b, COUNT);
        }
        const clock_t end = clock();
        if (begin == (clock_t)-1 || end == (clock_t)-1) {
            free(a); free(b);
            return 1;
        }
        const long double reference = dot_reference(a, b, COUNT);
        printf("%-16s CPU ms=%8.3f result=% .9g abs-error=%.9Lg\n", names[method],
               1000.0 * (double)(end - begin) / CLOCKS_PER_SEC,
               (double)sink, fabsl((long double)sink - reference));
    }
    free(a); free(b);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--benchmark"))) {
        fprintf(stderr, "Usage: %s [--benchmark]\n", argv[0]);
        return 1;
    }
    /* Volatile inputs keep the compiler-expression experiment from being
     * constant-folded before the selected target's instructions can matter.
     */
    volatile float a = 0x1.000002p0f, b = 0x1.fffffcp-1f, c = -1.0f;
    const float rounded = separate(a, b, c);
    const float fused = fmaf(a, b, c);
    if (rounded != 0.0f || fused != -0x1p-46f) {
        fputs("Unexpected binary32 rounding behavior\n", stderr);
        return 1;
    }
    printf("Separate multiply/add: %a; fused: %a; compiler expression: %a\n",
           (double)rounded, (double)fused, (double)expression(a, b, c));
    const float x[] = {1e8f, 1, -1e8f, 3};
    const float y[] = {1, 1, 1, 1};
    const long double reference = dot_reference(x, y, 4);
    const float sequential = dot_sequential(x, y, 4);
    const float regrouped = dot_two_accumulators(x, y, 4);
    if (reference != 4 || sequential != 3 || regrouped != 4 ||
        dot_sequential(x, y, 0) != 0 || dot_fused(x, y, 0) != 0 ||
        dot_two_accumulators(x, y, 3) != 1) {
        return 1;
    }
    printf("Cancellation example: sequential=%g, two accumulators=%g, reference=%Lg\n",
           (double)sequential, (double)regrouped, reference);
    return argc == 2 ? benchmark() : 0;
}
