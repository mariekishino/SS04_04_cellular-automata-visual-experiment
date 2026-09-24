#pragma once
// Minimal PNG writer with no dependencies: 8-bit RGB, zlib "stored" blocks
// (no compression). Enough for small frames; ffmpeg turns them into video.
#include <cstdint>
#include <string>
#include <vector>

namespace cave {

// rgb.size() must equal width * height * 3. Returns false on I/O failure.
bool write_png_rgb(const std::string& path, int width, int height, const std::vector<std::uint8_t>& rgb);

std::uint32_t crc32(const std::uint8_t* data, std::size_t len, std::uint32_t crc = 0);
std::uint32_t adler32(const std::uint8_t* data, std::size_t len);

}  // namespace cave
