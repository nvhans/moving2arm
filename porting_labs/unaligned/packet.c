#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint16_t kind;
    uint32_t length;
    uint16_t sequence;
} packet;

static uint32_t load_le32(const unsigned char *src) {
    uint32_t value;
    const uint16_t one = 1;
    /* Copy into an actual, aligned uint32_t object before using its value. */
    memcpy(&value, src, sizeof(value));
    if (*(const unsigned char *)&one == 1) {
        return value;
    }
    return ((value & UINT32_C(0xff)) << 24) |
           ((value & UINT32_C(0xff00)) << 8) |
           ((value >> 8) & UINT32_C(0xff00)) | (value >> 24);
}

static int parse_packet(const unsigned char *src, size_t size, packet *out) {
    if (size < 8) {
        return 0;
    }
    out->kind = (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8));
    out->length = load_le32(src + 2);
    out->sequence = (uint16_t)((uint16_t)src[6] | ((uint16_t)src[7] << 8));
    return 1;
}

int main(void) {
    const unsigned char wire[] = {0x34, 0x12, 0xef, 0xcd, 0xab, 0x89, 0xef, 0xbe};
    unsigned char storage[sizeof(wire) + 15];
    for (size_t offset = 0; offset < 16; ++offset) {
        memcpy(storage + offset, wire, sizeof(wire));
        packet parsed = {0, 0, 0};
        if (!parse_packet(storage + offset, sizeof(wire), &parsed) ||
            parsed.kind != 0x1234 || parsed.length != UINT32_C(0x89abcdef) ||
            parsed.sequence != 0xbeef) {
            fprintf(stderr, "Incorrect parsing at byte offset %zu\n", offset);
            return 1;
        }
        for (size_t size = 0; size < sizeof(wire); ++size) {
            const packet unchanged = parsed;
            if (parse_packet(storage + offset, size, &parsed) ||
                parsed.kind != unchanged.kind || parsed.length != unchanged.length ||
                parsed.sequence != unchanged.sequence) {
                fputs("Truncated packet accepted or output modified\n", stderr);
                return 1;
            }
        }
    }
    puts("Parsed a little-endian packet at offsets 0..15; rejected all truncated headers.");
    return 0;
}
