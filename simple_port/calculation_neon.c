#include <arm_neon.h>
#include "calculation.h"

/* AArch64 Advanced SIMD (NEON): four float lanes, plus a scalar tail.
 * Load order matches SSE array loads; there is no architecture lane reversal.
 */
void calculate(const float *a, const float *b, size_t n, calculation_output out) {
    size_t i = 0;
    for (; n - i >= 4; i += 4) {
        const float32x4_t va = vld1q_f32(a + i);
        const float32x4_t vb = vld1q_f32(b + i);
        const float32x4_t sum = vaddq_f32(va, vb);
        const float32x4_t product = vmulq_f32(sum, vb);
        /* NEON comparison lanes are all ones or zero; normalize to 1 or 0. */
        vst1q_u32(out.greater + i, vshrq_n_u32(vcgtq_f32(va, vb), 31));
        vst1q_f32(out.add + i, sum);
        vst1q_f32(out.mul + i, product);
        vst1q_f32(out.root + i, vsqrtq_f32(product));
    }
    calculate_scalar_range(a, b, i, n, out);
}

#include "calculation_demo.h"
