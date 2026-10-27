#include <nmmintrin.h>
#include "crc32c.h"

/* Only this translation unit is compiled with -msse4.2. */
uint32_t crc32c_hardware(const unsigned char *data, size_t n) {
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < n; ++i) {
        crc = _mm_crc32_u8(crc, data[i]);
    }
    return crc ^ UINT32_MAX;
}
