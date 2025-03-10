#include "xsimd/xsimd.hpp"
#include <stdio.h>

namespace xs = xsimd;

int main() {
    /* 
     * create two 4-element float vectors and assign to registers
     * NOTE: element order is reversed on Arm64
     */
    xs::batch<float> a = {1.0f, 4.0f, 9.0f, 16.0f};
    xs::batch<float> b = {1.0f, 2.0f, 3.0f, 4.0f};

    /*
     * compare vector a > vector b and store results in vector cmp_result
     * NOTE: extra step needed to cast vector of bools result from comparison to vector of float
     */
    xs::batch_bool<float> tmp = gt(a, b);
    xs::batch<float> cmp_result = select(tmp, xs::batch<float>(1.0f), xs::batch<float>(0.0f));

    /*
     * create vectors in CPU memory and store register values there
     */
    float a_arr[4], b_arr[4], cmp_arr[4];
    store(a_arr, a);
    store(b_arr, b);
    store(cmp_arr, cmp_result);

    for (int i = 0; i < 4; i++) {
        if (cmp_arr[i] != 0.0f) {
            printf("Element %d: %.2f is larger than %.2f\n", i, a_arr[i], b_arr[i]);
        } else {
            printf("Element %d: %.2f is not larger than %.2f\n", i, a_arr[i], b_arr[i]);
        }
    }

    /*
     * perform add, multiply, square root operations
     */
    xs::batch<float> add_result = add(a, b);
    xs::batch<float> mul_result = mul(add_result, b);
    xs::batch<float> sqrt_result = sqrt(mul_result);

    float res[4];

    /*
     * store arithmetic results in CPU memory for use in output text
     */
    store(res, add_result);
    printf("Addition Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    store(res, mul_result);
    printf("Multiplication Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    store(res, sqrt_result);
    printf("Square Root Result: %f %f %f %f\n", res[0], res[1], res[2], res[3]);

    return 0;
}
