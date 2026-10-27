#include <xsimd/xsimd.hpp>
#include "calculation.h"

/* The selected compiler target determines batch width, including on x86.
 * Unaligned transfers accept ordinary arrays without extra SIMD alignment.
 */
void calculate(const float *a, const float *b, size_t n, calculation_output out) {
    using batch = xsimd::batch<float>;
    constexpr size_t width = batch::size;
    size_t i = 0;
    for (; n - i >= width; i += width) {
        const batch va = batch::load_unaligned(a + i);
        const batch vb = batch::load_unaligned(b + i);
        const batch sum = va + vb;
        const batch product = sum * vb;
        float comparison[width];
        xsimd::select(va > vb, batch(1.0f), batch(0.0f)).store_unaligned(comparison);
        for (size_t lane = 0; lane < width; ++lane) {
            out.greater[i + lane] = static_cast<uint32_t>(comparison[lane]);
        }
        sum.store_unaligned(out.add + i);
        product.store_unaligned(out.mul + i);
        xsimd::sqrt(product).store_unaligned(out.root + i);
    }
    calculate_scalar_range(a, b, i, n, out);
}

#include "calculation_demo.h"
