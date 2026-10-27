#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8, "Wire format uses 8-bit bytes");
enum { WIRE_SIZE = 11 };
typedef struct {
    uint8_t version;
    uint32_t id;
    uint16_t flags;
    int32_t temperature_milli;
} record;

static void store_be32(unsigned char *out, uint32_t value) {
    out[0] = (unsigned char)(value >> 24);
    out[1] = (unsigned char)(value >> 16);
    out[2] = (unsigned char)(value >> 8);
    out[3] = (unsigned char)value;
}

static uint32_t load_be32(const unsigned char *in) {
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) |
           ((uint32_t)in[2] << 8) | (uint32_t)in[3];
}

static int encode(const record *value, unsigned char *out, size_t capacity) {
    if (capacity < WIRE_SIZE || value->version != 1) {
        return 0;
    }
    out[0] = value->version;
    store_be32(out + 1, value->id);
    out[5] = (unsigned char)(value->flags >> 8);
    out[6] = (unsigned char)value->flags;
    /* Conversion to unsigned is defined modulo 2^32, including negatives. */
    store_be32(out + 7, (uint32_t)value->temperature_milli);
    return 1;
}

static int decode(const unsigned char *in, size_t size, record *out) {
    if (size != WIRE_SIZE || in[0] != 1) {
        return 0;
    }
    const uint32_t bits = load_be32(in + 7);
    /* Avoid implementation-defined out-of-range unsigned-to-signed casts. */
    const int64_t signed_value = bits <= INT32_MAX ? (int64_t)bits :
                                (int64_t)bits - INT64_C(4294967296);
    const record value = {in[0], load_be32(in + 1),
        (uint16_t)(((uint16_t)in[5] << 8) | in[6]), (int32_t)signed_value};
    *out = value;
    return 1;
}

static int same(record a, record b) {
    return a.version == b.version && a.id == b.id && a.flags == b.flags &&
           a.temperature_milli == b.temperature_milli;
}

int main(void) {
    const record value = {1, UINT32_C(0x01020304), 0xa0b0, -1250};
    const unsigned char golden[WIRE_SIZE] = {1, 1, 2, 3, 4, 0xa0, 0xb0, 0xff, 0xff, 0xfb, 0x1e};
    unsigned char wire[WIRE_SIZE + 1];
    record decoded = {0, 0, 0, 0};
    if (!encode(&value, wire, sizeof(wire)) || memcmp(wire, golden, WIRE_SIZE) ||
        !decode(golden, sizeof(golden), &decoded) || !same(value, decoded)) {
        return 1;
    }
    for (size_t n = 0; n < WIRE_SIZE; ++n) {
        memset(wire, 0xcc, sizeof(wire));
        if (encode(&value, wire, n) || decode(golden, n, &decoded) || !same(value, decoded)) {
            return 1;
        }
        for (size_t i = 0; i < sizeof(wire); ++i) {
            if (wire[i] != 0xcc) {
                return 1;
            }
        }
    }
    memset(wire, 0, sizeof(wire));
    if (decode(wire, WIRE_SIZE, &decoded) || decode(wire, sizeof(wire), &decoded) ||
        !same(value, decoded)) {
        return 1;
    }
    const int32_t boundaries[] = {INT32_MIN, -1, 0, 1, INT32_MAX};
    for (size_t i = 0; i < sizeof(boundaries) / sizeof(boundaries[0]); ++i) {
        const record boundary = {1, UINT32_MAX, UINT16_MAX, boundaries[i]};
        if (!encode(&boundary, wire, sizeof(wire)) || !decode(wire, WIRE_SIZE, &decoded) ||
            !same(boundary, decoded)) {
            return 1;
        }
    }
    printf("In-memory layout: size=%zu alignment=%zu id-offset=%zu temperature-offset=%zu\n",
           sizeof(record), _Alignof(record), offsetof(record, id), offsetof(record, temperature_milli));
    printf("Stable %u-byte big-endian encoding:", (unsigned)WIRE_SIZE);
    for (size_t i = 0; i < sizeof(golden); ++i) {
        printf(" %02x", (unsigned)golden[i]);
    }
    puts("\nGolden bytes, truncation, version rejection, and signed boundaries passed.");
    return 0;
}
