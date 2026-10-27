#ifndef MOVING2ARM_CALCULATION_DEMO_H
#define MOVING2ARM_CALCULATION_DEMO_H

#include "calculation.h"

/* Keep each example directly compilable; tests supply their own main. */
#ifndef CALCULATION_NO_MAIN
#include <stdio.h>

int main(void) {
    const float a[] = {1.0f, 4.0f, 9.0f, 16.0f};
    const float b[] = {1.0f, 2.0f, 3.0f, 4.0f};
    uint32_t greater[4];
    float add[4], mul[4], root[4];
    const calculation_output out = {greater, add, mul, root};
    calculate(a, b, 4, out);

    for (size_t i = 0; i < 4; ++i) {
        printf("Element %zu: %.2f is %slarger than %.2f\n", i, a[i],
               greater[i] ? "" : "not ", b[i]);
    }
    printf("Addition Result: %f %f %f %f\n", add[0], add[1], add[2], add[3]);
    printf("Multiplication Result: %f %f %f %f\n", mul[0], mul[1], mul[2], mul[3]);
    printf("Square Root Result: %f %f %f %f\n", root[0], root[1], root[2], root[3]);
    return 0;
}
#endif

#endif
