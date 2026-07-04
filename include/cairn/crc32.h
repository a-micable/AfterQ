#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace cairn {
uint32_t crc32(const uint8_t* data, std::size_t size);
uint32_t crc32(const std::vector<uint8_t>& data);
}
