#include <arm_acle.h>
#include "crc32c.h"

/* CRC32C uses the Castagnoli polynomial. __crc32b computes a different CRC. */
uint32_t crc32c_hardware(const unsigned char *data, size_t n) {
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < n; ++i) {
        crc = __crc32cb(crc, data[i]);
    }
    return crc ^ UINT32_MAX;
}
