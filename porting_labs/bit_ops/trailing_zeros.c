#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static unsigned portable_ctz(uint64_t value) {
    if (!value) {
        return 64;
    }
    unsigned count = 0;
    while (!(value & UINT64_C(1))) {
        value >>= 1;
        ++count;
    }
    return count;
}

static unsigned builtin_ctz(uint64_t value) {
    /* __builtin_ctzll(0) has undefined behavior. Define the API's zero case. */
#if defined(__GNUC__) || defined(__clang__)
    return value ? (unsigned)__builtin_ctzll((unsigned long long)value) : 64u;
#else
    return portable_ctz(value);
#endif
}

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
static unsigned legacy_x86_ctz(uint64_t value) {
    if (!value) {
        return 64;
    }
    uint64_t result;
    __asm__("bsfq %1, %0" : "=r"(result) : "r"(value) : "cc");
    return (unsigned)result;
}
#endif

static int check(uint64_t value) {
    const unsigned expected = portable_ctz(value);
    if (builtin_ctz(value) != expected) {
        fprintf(stderr, "Builtin differs for %016" PRIx64 "\n", value);
        return 1;
    }
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
    if (legacy_x86_ctz(value) != expected) {
        return 1;
    }
#endif
    return 0;
}

int main(void) {
    if (check(0) || check(UINT64_MAX)) {
        return 1;
    }
    for (unsigned bit = 0; bit < 64; ++bit) {
        if (check(UINT64_C(1) << bit) || check(~(UINT64_C(1) << bit))) {
            return 1;
        }
    }
    uint64_t state = UINT64_C(0x123456789abcdef);
    for (unsigned i = 0; i < 10000; ++i) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        if (check(state)) {
            return 1;
        }
    }
    puts("ctz: zero maps to 64; single bits, complements, and 10000 patterns passed.");
    return 0;
}
