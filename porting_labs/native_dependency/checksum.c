#include "checksum.h"

uint64_t moving2arm_checksum(const unsigned char *data, size_t size) {
    uint64_t value = UINT64_C(0xcbf29ce484222325);
    for (size_t i = 0; i < size; ++i) {
        value ^= data[i];
        value *= UINT64_C(0x100000001b3);
    }
    return value;
}
