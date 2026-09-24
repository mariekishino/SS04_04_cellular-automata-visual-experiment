#pragma once
// load_audio: read WAV directly, or convert anything else (mp3, m4a, ogg, ...)
// with ffmpeg into a cached WAV next to the source, then read that.
//
// Name resolution so that files can be managed by dropping them in one
// directory:  "song"  ->  <audio_dir>/song.wav | song.mp3 | song.m4a | ...
#include <string>

#include "io/wav.hpp"

namespace cave {

struct AudioLoadInfo {
    std::string resolved_path;   // the file actually used as the source
    std::string wav_path;        // the WAV that was read (same as resolved_path for .wav)
    bool converted = false;      // true if ffmpeg ran (or a cached conversion was reused)
};

// audio_dir is searched when `name` is not an existing path.
bool load_audio(const std::string& name, const std::string& audio_dir, WavData& out, AudioLoadInfo* info, std::string* error);

// List playable files in a directory (wav + formats ffmpeg can convert), sorted.
std::string list_audio_dir(const std::string& audio_dir);

}  // namespace cave
