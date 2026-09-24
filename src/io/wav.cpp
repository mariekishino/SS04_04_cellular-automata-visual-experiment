#include "io/wav.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>

namespace cave {

namespace {
std::uint32_t le32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<std::uint32_t>(p[3]) << 24); }
std::uint16_t le16(const unsigned char* p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }
}  // namespace

bool read_wav(const std::string& path, WavData& out, std::string* error) {
    auto fail = [&](const char* msg) { if (error) *error = msg; return false; };
    std::ifstream f(path, std::ios::binary);
    if (!f) return fail("cannot open file");
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (buf.size() < 12 || std::memcmp(buf.data(), "RIFF", 4) != 0 || std::memcmp(buf.data() + 8, "WAVE", 4) != 0) return fail("not a RIFF/WAVE file");

    std::uint16_t format = 0, channels = 0, bits = 0;
    std::uint32_t rate = 0;
    const unsigned char* data = nullptr;
    std::uint32_t data_len = 0;
    std::size_t pos = 12;
    while (pos + 8 <= buf.size()) {
        const unsigned char* id = buf.data() + pos;
        const std::uint32_t len = le32(buf.data() + pos + 4);
        const std::size_t body = pos + 8;
        if (body + len > buf.size()) return fail("truncated chunk");
        if (std::memcmp(id, "fmt ", 4) == 0) {
            if (len < 16) return fail("fmt chunk too short");
            format = le16(buf.data() + body);
            channels = le16(buf.data() + body + 2);
            rate = le32(buf.data() + body + 4);
            bits = le16(buf.data() + body + 14);
            if (format == 0xFFFE && len >= 26) format = le16(buf.data() + body + 24);  // WAVE_FORMAT_EXTENSIBLE: sub-format
        } else if (std::memcmp(id, "data", 4) == 0) {
            data = buf.data() + body;
            data_len = len;
        }
        pos = body + len + (len & 1);  // chunks are word-aligned
    }
    if (!data) return fail("no data chunk");
    if (channels == 0 || rate == 0) return fail("no fmt chunk");
    const bool is_float = (format == 3);
    if (!(format == 1 || format == 3)) return fail("unsupported format (need PCM or IEEE float)");
    if (is_float && bits != 32) return fail("float WAV must be 32-bit");
    if (!is_float && !(bits == 16 || bits == 24 || bits == 32)) return fail("PCM must be 16, 24 or 32-bit");

    const std::size_t bytes_per_sample = bits / 8;
    const std::size_t frames = data_len / (bytes_per_sample * channels);
    out.sample_rate = static_cast<int>(rate);
    out.channels = channels;
    out.bits = bits;
    out.is_float = is_float;
    out.mono.assign(frames, 0.0f);
    for (std::size_t i = 0; i < frames; ++i) {
        double acc = 0.0;
        for (int c = 0; c < channels; ++c) {
            const unsigned char* p = data + (i * channels + c) * bytes_per_sample;
            double v = 0.0;
            if (is_float) { float fv; std::memcpy(&fv, p, 4); v = fv; }
            else if (bits == 16) v = static_cast<std::int16_t>(le16(p)) / 32768.0;
            else if (bits == 24) { std::int32_t s = (p[0] << 8) | (p[1] << 16) | (p[2] << 24); v = (s >> 8) / 8388608.0; }
            else v = static_cast<std::int32_t>(le32(p)) / 2147483648.0;
            acc += v;
        }
        out.mono[i] = static_cast<float>(acc / channels);
    }
    return true;
}

bool write_wav_pcm16_mono(const std::string& path, int sample_rate, const std::vector<float>& samples) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    auto put32 = [&](std::uint32_t v) { unsigned char b[4] = {static_cast<unsigned char>(v), static_cast<unsigned char>(v >> 8), static_cast<unsigned char>(v >> 16), static_cast<unsigned char>(v >> 24)}; f.write(reinterpret_cast<char*>(b), 4); };
    auto put16 = [&](std::uint16_t v) { unsigned char b[2] = {static_cast<unsigned char>(v), static_cast<unsigned char>(v >> 8)}; f.write(reinterpret_cast<char*>(b), 2); };
    const std::uint32_t data_len = static_cast<std::uint32_t>(samples.size() * 2);
    f.write("RIFF", 4); put32(36 + data_len); f.write("WAVE", 4);
    f.write("fmt ", 4); put32(16); put16(1); put16(1); put32(static_cast<std::uint32_t>(sample_rate)); put32(static_cast<std::uint32_t>(sample_rate * 2)); put16(2); put16(16);
    f.write("data", 4); put32(data_len);
    for (float s : samples) {
        const float c = s < -1.0f ? -1.0f : (s > 1.0f ? 1.0f : s);
        put16(static_cast<std::uint16_t>(static_cast<std::int16_t>(c * 32767.0f)));
    }
    return static_cast<bool>(f);
}

}  // namespace cave
