#ifndef MOVING2ARM_CALCULATION_H
#define MOVING2ARM_CALCULATION_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/* Each output points to n elements. Inputs and outputs must not overlap.
 * No SIMD alignment is required. A zero length must not access any buffer.
 * Comparisons are numeric 0/1, not floating-point representations of bit masks.
 */
typedef struct {
    uint32_t *greater;
    float *add;
    float *mul;
    float *root;
} calculation_output;

#ifdef __cplusplus
extern "C" {
#endif
void calculate(const float *a, const float *b, size_t n, calculation_output out);
#ifdef __cplusplus
}
#endif

/* Shared scalar tail; calculation.c also uses this as the portable baseline. */
static inline void calculate_scalar_range(const float *a, const float *b,
                                         size_t begin, size_t end,
                                         calculation_output out) {
    for (size_t i = begin; i < end; ++i) {
        out.greater[i] = a[i] > b[i] ? 1u : 0u;
        out.add[i] = a[i] + b[i];
        out.mul[i] = out.add[i] * b[i];
        out.root[i] = sqrtf(out.mul[i]);
    }
}

#endif
