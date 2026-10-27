#include "crc32c.h"

uint32_t crc32c_portable(const unsigned char *data, size_t n) {
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < n; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1u) ? UINT32_C(0x82f63b78) : 0u);
        }
    }
    return crc ^ UINT32_MAX;
}
