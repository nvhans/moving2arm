/* replacing with SIMDE #include <xmmintrin.h> */
#include <simde/x86/sse.h>
#include <stdio.h>

int main() {
    simde__m128 a = simde_mm_set_ps(16.0f, 9.0f, 4.0f, 1.0f);
    simde__m128 b = simde_mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);

    simde__m128 cmp_result = simde_mm_cmpgt_ps(a, b);

    float a_arr[4], b_arr[4], cmp_arr[4];
    simde_mm_storeu_ps(a_arr, a);
    simde_mm_storeu_ps(b_arr, b);
    simde_mm_storeu_ps(cmp_arr, cmp_result);

    for (int i = 0; i < 4; i++) {
        if (cmp_arr[i] != 0.0f) {
            printf("Element %d: %.2f is larger than %.2f\n", i, a_arr[i], b_arr[i]);
        } else {
            printf("Element %d: %.2f is not larger than %.2f\n", i, a_arr[i], b_arr[i]);
        }
    }

    simde__m128 add_result = simde_mm_add_ps(a, b);
    simde__m128 mul_result = simde_mm_mul_ps(add_result, b);
    simde__m128 sqrt_result = simde_mm_sqrt_ps(mul_result);

    float res[4];

    simde_mm_storeu_ps(res, add_result);
    printf("Addition Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    simde_mm_storeu_ps(res, mul_result);
    printf("Multiplication Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    simde_mm_storeu_ps(res, sqrt_result);
    printf("Square Root Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    return 0;
}
