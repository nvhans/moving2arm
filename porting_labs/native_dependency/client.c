#include "checksum.h"
#include <inttypes.h>
#include <stdio.h>

int main(void) {
    const unsigned char input[] = "hello";
    const uint64_t result = moving2arm_checksum(input, 5);
    if (result != UINT64_C(0xa430d84680aabd0b) ||
        moving2arm_checksum(NULL, 0) != UINT64_C(0xcbf29ce484222325)) {
        return 1;
    }
    printf("Loaded the native shared library; FNV-1a(hello)=%016" PRIx64 "\n", result);
    return 0;
}
