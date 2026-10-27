#include "calculation.h"

/* Portable reference, also suitable for compiler auto-vectorization exercises. */
void calculate(const float *a, const float *b, size_t n, calculation_output out) {
    calculate_scalar_range(a, b, 0, n, out);
}

#include "calculation_demo.h"
