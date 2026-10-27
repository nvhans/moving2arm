#include <xmmintrin.h>
#include "calculation.h"

/* Array loads preserve element order. _mm_set_ps instead lists the highest
 * lane first; this is a constructor convention, not an Arm/x86 difference.
 * The original x86 intrinsics belong to SSE.
 */
void calculate(const float *a, const float *b, size_t n, calculation_output out) {
    size_t i = 0;
    for (; n - i >= 4; i += 4) {
        const __m128 va = _mm_loadu_ps(a + i);
        const __m128 vb = _mm_loadu_ps(b + i);
        const __m128 sum = _mm_add_ps(va, vb);
        const __m128 product = _mm_mul_ps(sum, vb);
        const unsigned mask = (unsigned)_mm_movemask_ps(_mm_cmpgt_ps(va, vb));
        for (unsigned lane = 0; lane < 4; ++lane) {
            out.greater[i + lane] = (mask >> lane) & 1u;
        }
        _mm_storeu_ps(out.add + i, sum);
        _mm_storeu_ps(out.mul + i, product);
        _mm_storeu_ps(out.root + i, _mm_sqrt_ps(product));
    }
    calculate_scalar_range(a, b, i, n, out);
}

#include "calculation_demo.h"
