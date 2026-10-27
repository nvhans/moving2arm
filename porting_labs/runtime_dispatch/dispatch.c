#include "crc32c.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#if defined(LAB_CRC_ARM) && defined(__linux__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#elif defined(LAB_CRC_ARM) && defined(__APPLE__)
#include <sys/sysctl.h>
#endif

static int hardware_available(void) {
#if defined(LAB_CRC_X86)
    __builtin_cpu_init();
    return __builtin_cpu_supports("sse4.2") != 0;
#elif defined(LAB_CRC_ARM) && defined(__linux__)
    return (getauxval(AT_HWCAP) & HWCAP_CRC32) != 0;
#elif defined(LAB_CRC_ARM) && defined(__APPLE__)
    int supported = 0;
    size_t size = sizeof(supported);
    return sysctlbyname("hw.optional.armv8_crc32", &supported, &size, NULL, 0) == 0 && supported;
#else
    return 0;
#endif
}

int main(int argc, char **argv) {
    const char *backend = argc > 1 ? argv[1] : "auto";
    const int disabled = argc == 3 && !strcmp(argv[2], "--disable-hardware");
    if (argc > 3 || (argc == 3 && !disabled) ||
        (strcmp(backend, "auto") && strcmp(backend, "portable") && strcmp(backend, "hardware"))) {
        fprintf(stderr, "Usage: %s [auto|portable|hardware] [--disable-hardware]\n", argv[0]);
        return 1;
    }
    const int available = !disabled && hardware_available();
    if (!strcmp(backend, "hardware") && !available) {
        puts("Skipping: hardware CRC32C isn't available.");
        return 77;
    }
    uint32_t (*checksum)(const unsigned char *, size_t) = crc32c_portable;
    const char *selected = "portable";
#if defined(LAB_CRC_X86) || defined(LAB_CRC_ARM)
    if (available && strcmp(backend, "portable")) {
        checksum = crc32c_hardware;
        selected = "hardware";
    }
#endif
    const unsigned char golden[] = "123456789";
    if (checksum(golden, 9) != UINT32_C(0xe3069283) || checksum(NULL, 0) != 0) {
        fputs("CRC32C golden vector failed\n", stderr);
        return 1;
    }
    unsigned char data[258];
    for (size_t i = 0; i < sizeof(data); ++i) {
        data[i] = (unsigned char)(i * 37u);
    }
    for (size_t n = 0; n <= 257; ++n) {
        if (checksum(data + 1, n) != crc32c_portable(data + 1, n)) {
            fprintf(stderr, "CRC32C mismatch at length %zu\n", n);
            return 1;
        }
    }
    printf("CRC32C backend: %s; golden checksum: %08" PRIx32 "; lengths 0..257 passed.\n",
           selected, checksum(golden, 9));
    return 0;
}
