/* Explicit SIMDe names avoid introducing x86 aliases. */
#include <simde/x86/sse.h>
#include "calculation.h"

/* Array loads preserve element order. _mm_set_ps instead lists the highest
 * lane first; this is a constructor convention, not an Arm/x86 difference.
 * The original x86 intrinsics belong to SSE.
 */
void calculate(const float *a, const float *b, size_t n, calculation_output out) {
    size_t i = 0;
    for (; n - i >= 4; i += 4) {
        const simde__m128 va = simde_mm_loadu_ps(a + i);
        const simde__m128 vb = simde_mm_loadu_ps(b + i);
        const simde__m128 sum = simde_mm_add_ps(va, vb);
        const simde__m128 product = simde_mm_mul_ps(sum, vb);
        const unsigned mask = (unsigned)simde_mm_movemask_ps(simde_mm_cmpgt_ps(va, vb));
        for (unsigned lane = 0; lane < 4; ++lane) {
            out.greater[i + lane] = (mask >> lane) & 1u;
        }
        simde_mm_storeu_ps(out.add + i, sum);
        simde_mm_storeu_ps(out.mul + i, product);
        simde_mm_storeu_ps(out.root + i, simde_mm_sqrt_ps(product));
    }
    calculate_scalar_range(a, b, i, n, out);
}

#include "calculation_demo.h"
