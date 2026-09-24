#include <cstdio>
#include <fstream>
#include <vector>
#include "io/png.hpp"
#include "io/colormap.hpp"
#include "check.hpp"
using namespace cave;
int main() {
    // known CRC32 / Adler32 test vectors ("123456789")
    const unsigned char s[] = "123456789";
    CHECK(crc32(s, 9) == 0xCBF43926u);
    CHECK(adler32(s, 9) == 0x091E01DEu);

    Grid g(3, 2, 0.0f);
    g.at(2, 1) = 1.0f;
    std::vector<std::uint8_t> rgb = render_rgb(g, 2, Colormap::Gray, 0.0f, 1.0f);
    CHECK(rgb.size() == 6 * 4 * 3);
    CHECK(rgb[0] == 0);
    CHECK(rgb[rgb.size() - 1] == 255);  // bottom-right pixel from cell (2,1)
    const std::string path = "test_png_out.png";
    CHECK(write_png_rgb(path, 6, 4, rgb));
    std::ifstream f(path, std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(bytes.size() > 33);
    CHECK(bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G');
    // IHDR width/height big-endian at offsets 16..23
    CHECK(bytes[19] == 6 && bytes[23] == 4);
    std::remove(path.c_str());
    std::puts("test_png OK");
    return 0;
}
