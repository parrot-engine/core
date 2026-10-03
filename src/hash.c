#include "parrot/core/hash.h"

ParrotCRC32 Parrot_crc32(const void *data, size_t size) {
    return Parrot_crc32_combine(~0xFFFFFFFF, data, size);
}

ParrotCRC32 Parrot_crc32_combine(ParrotCRC32 crc, const void *void_data, size_t size) {
    const uint8_t *data = (uint8_t *)void_data;

    PARROT_FAIL_NULL(data);

    crc = ~crc;

    while (size-- > 0) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++) {
            const uint32_t poly = 0xEDB88320;

            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (mask & poly);
        }
    }

    return ~crc;
}
