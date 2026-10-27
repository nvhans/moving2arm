#include <limits.h>
#include <stdio.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "This byte-format exercise requires 8-bit bytes");

/* Legacy code silently interprets high bytes according to plain-char signedness. */
static int legacy_sum(const char *bytes, size_t n) {
    int result = 0;
    for (size_t i = 0; i < n; ++i) {
        result += bytes[i];
    }
    return result;
}

static unsigned byte_sum(const char *bytes, size_t n) {
    unsigned result = 0;
    for (size_t i = 0; i < n; ++i) {
        result += (unsigned char)bytes[i];
    }
    return result;
}

int main(void) {
    unsigned char values[256];
    char input[256];
    for (unsigned i = 0; i < 256; ++i) {
        values[i] = (unsigned char)i;
    }
    memcpy(input, values, sizeof(input));
    for (unsigned i = 0; i < 256; ++i) {
        if (byte_sum(input + i, 1) != i) {
            fprintf(stderr, "Byte %u changed meaning\n", i);
            return 1;
        }
    }
    const int legacy = legacy_sum(input, sizeof(input));
    const unsigned portable = byte_sum(input, sizeof(input));
    const int expected_legacy = CHAR_MIN < 0 ? -128 : 32640;
    if (portable != 32640 || legacy != expected_legacy) {
        return 1;
    }
    printf("Plain char is %s: legacy sum = %d; portable byte sum = %u\n",
           CHAR_MIN < 0 ? "signed" : "unsigned", legacy, portable);
    return 0;
}
