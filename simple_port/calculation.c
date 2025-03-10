#include <math.h> // for sqrt()
#include <stdio.h>

int main() {
    /*
     * Create two 4-element vectors
     */	
    float a[4] = {1.0f, 4.0f, 9.0f, 16.0f};
    float b[4] = {1.0f, 2.0f, 3.0f, 4.0f};

    /*
     * create 4-element vector & compare a>b element-wise the two vectors above
     */
    float cmp_result[4];
    for (int i = 0; i < 4; i++) {
	float x;
        if (a>b){
	    cmp_result[i] = 1.0f;
	} else {
	    cmp_result[i] = 0.0f;
	}
    }


    /*
     * iterate through comparison results
     */
    for (int i = 0; i < 4; i++) {
        if (cmp_result[i] != 0.0f) {
            printf("Element %d: %.2f is larger than %.2f\n", i, a[i], b[i]);
        } else {
            printf("Element %d: %.2f is not larger than %.2f\n", i, a[i], b[i]);
        }
    }

    /*
     * perform element-wise addition, multiplication, and square-root
     */
    float add_result[4], mul_result[4], sqrt_result[4];
    for (int i = 0; i < 4; i++) {
	add_result[i] = a[i] + b[i];
	mul_result[i] = add_result[i] * b[i];
	sqrt_result[i] = sqrt(mul_result[i]);
    }


    /*
     * output results
     */
    printf("Addition Result: %f %f %f %f\n", add_result[0], add_result[1], add_result[2], add_result[3]);

    printf("Multiplication Result: %f %f %f %f\n", mul_result[0], mul_result[1], mul_result[2], mul_result[3]);

    printf("Square Root Result: %f %f %f %f\n", sqrt_result[0], sqrt_result[1], sqrt_result[2], sqrt_result[3]);

    return 0;
}
