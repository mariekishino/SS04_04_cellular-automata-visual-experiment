#pragma once
// Minimal RIFF/WAVE reader: PCM 16/24/32-bit and IEEE float 32-bit, any channel
// count (mixed down to mono by averaging). No dependencies. MP3 and others are
// converted beforehand with scripts/to_wav.sh (ffmpeg).
#include <string>
#include <vector>

namespace cave {

struct WavData {
    int sample_rate = 0;
    int channels = 0;
    int bits = 0;
    bool is_float = false;
    std::vector<float> mono;   // samples in [-1, 1], averaged over channels
};

bool read_wav(const std::string& path, WavData& out, std::string* error);

// 16-bit PCM mono writer, for tests and tools.
bool write_wav_pcm16_mono(const std::string& path, int sample_rate, const std::vector<float>& samples);

}  // namespace cave
