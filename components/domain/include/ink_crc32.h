#pragma once

#include <stddef.h>
#include <stdint.h>

/* CRC-32 (IEEE 802.3, đa thức 0xEDB88320), giống zlib/Python binascii.crc32. */
uint32_t ink_crc32(const void *data, size_t len);
/* Tính nối tiếp: crc ban đầu = 0. */
uint32_t ink_crc32_update(uint32_t crc, const void *data, size_t len);
