#ifndef MOVING2ARM_NATIVE_CHECKSUM_H
#define MOVING2ARM_NATIVE_CHECKSUM_H
#include <stddef.h>
#include <stdint.h>
/* FNV-1a with defined unsigned wraparound. Rebuild for each platform's C ABI. */
uint64_t moving2arm_checksum(const unsigned char *data, size_t size);
#endif
