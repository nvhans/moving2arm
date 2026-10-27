#include <arm_sve.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* SVE2 adds the predicated signed saturating-add form used here.
 * Clamp overflow instead of wrapping. Unpredicated svqadd_s32 is base SVE.
 */
static void saturating_add(const int32_t *a, const int32_t *b, int32_t *result,
                           size_t n) {
    for (size_t i = 0; i < n; i += svcntw()) {
        const svbool_t pg = svwhilelt_b32_u64((uint64_t)i, (uint64_t)n);
        const svint32_t va = svld1_s32(pg, a + i);
        const svint32_t vb = svld1_s32(pg, b + i);
        svst1_s32(pg, result + i, svqadd_s32_m(pg, va, vb));
    }
}

int main(void) {
    const size_t bits = svcntb() * 8;
    const char *expected_bits = getenv("MOVING2ARM_EXPECT_SVE_BITS");
    if (expected_bits && strtoul(expected_bits, NULL, 10) != bits) {
        fprintf(stderr, "Expected %s-bit SVE vectors, got %zu bits\n", expected_bits, bits);
        return 1;
    }
    printf("SVE vector length: %zu bits\n", bits);
    const int32_t pattern_a[] = {1, 2, 3, 4, 5, 6, 7, 8, INT32_MAX, INT32_MIN};
    const int32_t pattern_b[] = {8, 7, 6, 5, 4, 3, 2, 1, 1, -1};
    int32_t a[131], b[131], result[133];
    const size_t count = sizeof(a) / sizeof(a[0]);
    for (size_t i = 0; i < count; ++i) {
        a[i] = pattern_a[i % 10];
        b[i] = pattern_b[i % 10];
    }
    for (size_t n = 0; n <= count; ++n) {
        for (size_t i = 0; i < count + 2; ++i) {
            result[i] = -1234567;
        }
        saturating_add(a, b, result + 1, n);
        for (size_t i = 0; i < n; ++i) {
            const int64_t sum = (int64_t)a[i] + (int64_t)b[i];
            const int32_t expected = sum > INT32_MAX ? INT32_MAX :
                                     sum < INT32_MIN ? INT32_MIN : (int32_t)sum;
            if (result[i + 1] != expected) {
                fprintf(stderr, "Length %zu: saturating addition failed at %zu\n", n, i);
                return 1;
            }
        }
        if (result[0] != -1234567 || result[n + 1] != -1234567) {
            fprintf(stderr, "Length %zu: output guard overwritten\n", n);
            return 1;
        }
    }
    printf("First eight sums: ");
    for (size_t i = 1; i <= 8; ++i) {
        printf("%" PRId32 "%s", result[i], i == 8 ? "\n" : " ");
    }
    printf("Saturated boundaries: %" PRId32 " %" PRId32 "\n", result[9], result[10]);
    puts("Passed SVE2 saturating addition for lengths 0..131.");
    return 0;
}
