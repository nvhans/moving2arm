#ifndef MOVING2ARM_CRC32C_H
#define MOVING2ARM_CRC32C_H
#include <stddef.h>
#include <stdint.h>
uint32_t crc32c_portable(const unsigned char *data, size_t n);
#if defined(LAB_CRC_X86) || defined(LAB_CRC_ARM)
uint32_t crc32c_hardware(const unsigned char *data, size_t n);
#endif
#endif
