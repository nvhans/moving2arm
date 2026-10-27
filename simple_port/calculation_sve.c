#include <arm_sve.h>
#include "calculation.h"

/* SVE uses a scalable vector type, not a fixed count of float lanes.
 * The predicate protects every load/store, including the final partial vector.
 */
void calculate(const float *a, const float *b, size_t n, calculation_output out) {
    for (size_t i = 0; i < n; i += svcntw()) {
        const svbool_t pg = svwhilelt_b32_u64((uint64_t)i, (uint64_t)n);
        const svfloat32_t va = svld1_f32(pg, a + i);
        const svfloat32_t vb = svld1_f32(pg, b + i);
        const svbool_t greater = svcmpgt_f32(pg, va, vb);
        const svfloat32_t sum = svadd_f32_x(pg, va, vb);
        const svfloat32_t product = svmul_f32_x(pg, sum, vb);
        const svuint32_t comparison = svsel_u32(greater, svdup_u32(1), svdup_u32(0));
        svst1_u32(pg, out.greater + i, comparison);
        svst1_f32(pg, out.add + i, sum);
        svst1_f32(pg, out.mul + i, product);
        svst1_f32(pg, out.root + i, svsqrt_f32_x(pg, product));
    }
}

#include "calculation_demo.h"
