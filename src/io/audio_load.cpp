#include "io/audio_load.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <vector>

#include <sys/stat.h>
#include <dirent.h>

namespace cave {

namespace {

const char* kConvertible[] = {".mp3", ".m4a", ".aac", ".ogg", ".flac", ".aiff", ".aif", ".wma", ".opus"};

bool exists(const std::string& p, time_t* mtime = nullptr) {
    struct stat st{};
    if (::stat(p.c_str(), &st) != 0) return false;
    if (mtime) *mtime = st.st_mtime;
    return true;
}

std::string lower_ext(const std::string& p) {
    const std::size_t dot = p.rfind('.');
    const std::size_t slash = p.rfind('/');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return "";
    std::string e = p.substr(dot);
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return e;
}

bool is_convertible(const std::string& ext) {
    for (const char* c : kConvertible) if (ext == c) return true;
    return false;
}

std::string shell_quote(const std::string& s) {
    std::string q = "'";
    for (char c : s) { if (c == '\'') q += "'\\''"; else q += c; }
    return q + "'";
}

std::string resolve(const std::string& name, const std::string& audio_dir) {
    if (exists(name)) return name;
    if (name.find('/') != std::string::npos) return "";
    const std::string base = audio_dir + "/" + name;
    if (exists(base)) return base;
    if (exists(base + ".wav")) return base + ".wav";
    for (const char* c : kConvertible) if (exists(base + c)) return base + c;
    return "";
}

}  // namespace

bool load_audio(const std::string& name, const std::string& audio_dir, WavData& out, AudioLoadInfo* info, std::string* error) {
    auto fail = [&](const std::string& msg) { if (error) *error = msg; return false; };
    const std::string path = resolve(name, audio_dir);
    if (path.empty()) return fail("not found: " + name + " (also looked in " + audio_dir + "/ with .wav/.mp3/... appended)");
    AudioLoadInfo li;
    li.resolved_path = path;
    const std::string ext = lower_ext(path);
    if (ext == ".wav") {
        li.wav_path = path;
    } else if (is_convertible(ext)) {
        // cache: <path without ext>.wav, rebuilt if older than the source
        const std::string wav = path.substr(0, path.size() - ext.size()) + ".wav";
        time_t src_t = 0, wav_t = 0;
        exists(path, &src_t);
        const bool cached = exists(wav, &wav_t) && wav_t >= src_t;
        if (!cached) {
            const std::string cmd = "ffmpeg -loglevel error -y -i " + shell_quote(path) + " -ac 1 -ar 44100 -sample_fmt s16 " + shell_quote(wav);
            const int rc = std::system(cmd.c_str());
            if (rc != 0) return fail("ffmpeg failed (is ffmpeg installed? macOS: brew install ffmpeg) converting " + path);
        }
        li.wav_path = wav;
        li.converted = true;
    } else {
        return fail("unsupported extension '" + ext + "' for " + path);
    }
    std::string werr;
    if (!read_wav(li.wav_path, out, &werr)) return fail(li.wav_path + ": " + werr);
    if (info) *info = li;
    return true;
}

std::string list_audio_dir(const std::string& audio_dir) {
    std::vector<std::string> names;
    if (DIR* d = ::opendir(audio_dir.c_str())) {
        while (dirent* e = ::readdir(d)) {
            const std::string n = e->d_name;
            const std::string ext = lower_ext(n);
            if (ext == ".wav" || is_convertible(ext)) names.push_back(n);
        }
        ::closedir(d);
    }
    std::sort(names.begin(), names.end());
    std::ostringstream os;
    for (const std::string& n : names) os << "  " << n << "\n";
    return os.str();
}

}  // namespace cave
