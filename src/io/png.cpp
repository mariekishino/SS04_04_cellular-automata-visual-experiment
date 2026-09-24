#include "io/png.hpp"

#include <fstream>

namespace cave {

namespace {

void put_be32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v >> 24));
    out.push_back(static_cast<std::uint8_t>(v >> 16));
    out.push_back(static_cast<std::uint8_t>(v >> 8));
    out.push_back(static_cast<std::uint8_t>(v));
}

void put_chunk(std::vector<std::uint8_t>& out, const char type[4], const std::vector<std::uint8_t>& data) {
    put_be32(out, static_cast<std::uint32_t>(data.size()));
    std::vector<std::uint8_t> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    out.insert(out.end(), td.begin(), td.end());
    put_be32(out, crc32(td.data(), td.size()));
}

}  // namespace

std::uint32_t crc32(const std::uint8_t* data, std::size_t len, std::uint32_t crc) {
    static std::uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[n] = c;
        }
        init = true;
    }
    crc ^= 0xFFFFFFFFu;
    for (std::size_t i = 0; i < len; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

std::uint32_t adler32(const std::uint8_t* data, std::size_t len) {
    std::uint32_t a = 1, b = 0;
    for (std::size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

bool write_png_rgb(const std::string& path, int width, int height, const std::vector<std::uint8_t>& rgb) {
    if (width <= 0 || height <= 0) return false;
    const std::size_t row_bytes = static_cast<std::size_t>(width) * 3;
    if (rgb.size() != row_bytes * static_cast<std::size_t>(height)) return false;

    // Raw scanlines: filter byte 0 (None) + RGB bytes.
    std::vector<std::uint8_t> raw;
    raw.reserve((row_bytes + 1) * static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        const std::uint8_t* row = rgb.data() + static_cast<std::size_t>(y) * row_bytes;
        raw.insert(raw.end(), row, row + row_bytes);
    }

    // zlib stream with stored (uncompressed) deflate blocks of <= 65535 bytes.
    std::vector<std::uint8_t> z;
    z.push_back(0x78);
    z.push_back(0x01);
    std::size_t pos = 0;
    while (pos < raw.size()) {
        const std::size_t n = std::min<std::size_t>(65535, raw.size() - pos);
        const bool last = (pos + n == raw.size());
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<std::uint8_t>(n & 0xFF));
        z.push_back(static_cast<std::uint8_t>(n >> 8));
        z.push_back(static_cast<std::uint8_t>(~n & 0xFF));
        z.push_back(static_cast<std::uint8_t>((~n >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + static_cast<std::ptrdiff_t>(pos), raw.begin() + static_cast<std::ptrdiff_t>(pos + n));
        pos += n;
    }
    put_be32(z, adler32(raw.data(), raw.size()));

    std::vector<std::uint8_t> out = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::vector<std::uint8_t> ihdr;
    put_be32(ihdr, static_cast<std::uint32_t>(width));
    put_be32(ihdr, static_cast<std::uint32_t>(height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(2);  // color type: RGB
    ihdr.push_back(0);  // compression
    ihdr.push_back(0);  // filter
    ihdr.push_back(0);  // interlace
    put_chunk(out, "IHDR", ihdr);
    put_chunk(out, "IDAT", z);
    put_chunk(out, "IEND", {});

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(f);
}

}  // namespace cave
