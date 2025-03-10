/* This file represents re-writing all x86 intrinsics using aarch64 intrinsics
 * intrinsics included:
 * 	__SSE4.2__	>>	__NEON__
 * 	_mm_cmpgt_ps 	>> 	vcgtq_f32
 * 	_mm_add_ps	>>	vaddq_f32
 * 	_mm_mul_ps	>>	vmulq_f32
 * 	_mm_sqrt_ps	>>	vsqrtq_f32
 */
#include <arm_sve.h> // replacing <xmmintrin.h> 
#include <stdio.h>

int main() {
    /* Initiate vector values and load into registers
     * NOTE we reverse the order of vector values before loading
     * __m128 a = _mm_set_ps(16.0f, 9.0f, 4.0f, 1.0f); 
     */
    float32_t _a[] = {1.0f, 4.0f, 9.0f, 16.0f};
    svfloat32x4_t a = svld1_f32(svptrue_b32(), _a);

    /* __m128 b = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f); */
    float32_t _b[] = {4.0f, 3.0f, 2.0f, 1.0f}; 
    svfloat32x4_t b = svld1_f32(svptrue_b32(), _b);

    /* element-wise greater-than comparison
     * __m128 cmp_result = _mm_cmpgt_ps(a, b);
     */
    svuint32x4_t cmp_result = svcgtq_f32(a, b);

    float32_t a_arr[4], b_arr[4];
    uint32_t cmp_arr[4];

    /* store vectors in CPU memory
     * _mm_storeu_ps(a_arr, a);
     * _mm_storeu_ps(b_arr, b);
     * _mm_storeu_ps(cmp_arr, cmp_result);
     */
    vst1q_f32(a_arr, a);
    vst1q_f32(b_arr, b);
    vst1q_u32(cmp_arr, cmp_result);

    for (int i = 0; i < 4; i++) {
        if (cmp_arr[i] != 0.0f) {
            printf("Element %d: %.2f is larger than %.2f\n", i, a_arr[i], b_arr[i]);
        } else {
            printf("Element %d: %.2f is not larger than %.2f\n", i, a_arr[i], b_arr[i]);
        }
    }

    /* perform add, multiplication, and find square-roots functions
     * __m128 add_result = _mm_add_ps(a, b);
     * __m128 mul_result = _mm_mul_ps(add_result, b);   
     * __m128 sqrt_result = _mm_sqrt_ps(mul_result);
     */
    float32x4_t add_result = vaddq_f32(a, b);
    float32x4_t mul_result = vmulq_f32(add_result, b);
    float32x4_t sqrt_result = vsqrtq_f32(mul_result);

    float32_t res[4]; // 4-element vector for printing results

    /* store results
     * _mm_storeu_ps(res, add_result);
     */
    vst1q_f32(res, add_result);
    printf("Addition Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    /* store results _mm_storeu_ps(res, mul_result); */
    vst1q_f32(res, mul_result);
    printf("Multiplication Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    /* store results _mm_storeu_ps(res, sqrt_result); */
    vst1q_f32(res, sqrt_result);
    printf("Square Root Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    return 0;
}
