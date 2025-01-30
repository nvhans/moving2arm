/* replacing with SIMDE #include <xmmintrin.h> */
#define SIMDE_ENABLE_NATIVE_ALIASES
#include <simde/x86/sse.h>
#include <stdio.h>

int main() {
    __m128 a = _mm_set_ps(16.0f, 9.0f, 4.0f, 1.0f);
    __m128 b = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);

    __m128 cmp_result = _mm_cmpgt_ps(a, b);

    float a_arr[4], b_arr[4], cmp_arr[4];
    _mm_storeu_ps(a_arr, a);
    _mm_storeu_ps(b_arr, b);
    _mm_storeu_ps(cmp_arr, cmp_result);

    for (int i = 0; i < 4; i++) {
        if (cmp_arr[i] != 0.0f) {
            printf("Element %d: %.2f is larger than %.2f\n", i, a_arr[i], b_arr[i]);
        } else {
            printf("Element %d: %.2f is not larger than %.2f\n", i, a_arr[i], b_arr[i]);
        }
    }

    __m128 add_result = _mm_add_ps(a, b);
    __m128 mul_result = _mm_mul_ps(add_result, b);
    __m128 sqrt_result = _mm_sqrt_ps(mul_result);

    float res[4];

    _mm_storeu_ps(res, add_result);
    printf("Addition Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    _mm_storeu_ps(res, mul_result);
    printf("Multiplication Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    _mm_storeu_ps(res, sqrt_result);
    printf("Square Root Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    return 0;
}
