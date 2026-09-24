// cave_window: Phase 1 standalone app. Draws the model in an SDL3 window.
//
// The simulation runs at a fixed time step. Wall-clock time is accumulated
// and converted into an integer number of steps per frame (steps-per-second
// is a setting), so rendering frame rate and simulation rate are independent.
//
// Keys:  Space pause/resume   N one step (while paused)   R reset (same seed)
//        S screenshot          + / - double / halve steps per second
//        Q or Esc quit
//
// Phase 2: --wav FILE plays the file and drives the stimulus from the audio
// clock (bytes played so far), so what you hear and what the model receives
// stay aligned. --stim-mode/--stim-gain/--stim-shape choose the entry point.
//
// Headless verification (no keyboard):  --frames N quits after N frames,
// --keys "60:space,70:n,..." injects key presses at given frames,
// --shot-dir DIR saves a screenshot every --shot-every frames.

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>

#include "core/input.hpp"
#include "core/metrics.hpp"
#include "core/model.hpp"
#include "core/presets.hpp"
#include "io/colormap.hpp"
#include "io/png.hpp"
#include "io/wav.hpp"

using namespace cave;

namespace {

struct Args {
    std::string model = "lenia", preset = "orbium", init = "orbium";
    unsigned seed = 1;
    int width = 64, height = 64, scale = 8;
    double sps = 60.0;         // simulation steps per second
    float threshold = 0.1f, vmax = 1.0f;
    Colormap colormap = Colormap::Viridis;
    std::string shot_dir = "experiments/out/window";
    long frames = -1;          // quit after N frames (<0: run until quit)
    long shot_every = 0;       // 0: only on key S
    std::vector<std::pair<long, std::string>> keys;
    double report_seconds = 1.0;
    // Phase 2
    std::string wav_path;
    Overrides overrides;                 // stim_mode, stim_gain
    StimulusShape stim_shape = StimulusShape::Uniform;
    double smooth_tau = 0.05;
};

bool parse(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i], v;
        auto need = [&](std::string& out) { if (i + 1 >= argc) return false; out = argv[++i]; return true; };
        if (k == "--model") { if (!need(a.model)) return false; }
        else if (k == "--preset") { if (!need(a.preset)) return false; }
        else if (k == "--init") { if (!need(a.init)) return false; }
        else if (k == "--seed") { if (!need(v)) return false; a.seed = static_cast<unsigned>(std::stoul(v)); }
        else if (k == "--width") { if (!need(v)) return false; a.width = std::stoi(v); }
        else if (k == "--height") { if (!need(v)) return false; a.height = std::stoi(v); }
        else if (k == "--scale") { if (!need(v)) return false; a.scale = std::stoi(v); }
        else if (k == "--sps") { if (!need(v)) return false; a.sps = std::stod(v); }
        else if (k == "--threshold") { if (!need(v)) return false; a.threshold = std::stof(v); }
        else if (k == "--vmax") { if (!need(v)) return false; a.vmax = std::stof(v); }
        else if (k == "--colormap") { if (!need(v)) return false; a.colormap = colormap_from_name(v); }
        else if (k == "--shot-dir") { if (!need(a.shot_dir)) return false; }
        else if (k == "--frames") { if (!need(v)) return false; a.frames = std::stol(v); }
        else if (k == "--shot-every") { if (!need(v)) return false; a.shot_every = std::stol(v); }
        else if (k == "--report") { if (!need(v)) return false; a.report_seconds = std::stod(v); }
        else if (k == "--wav") { if (!need(a.wav_path)) return false; }
        else if (k == "--stim-mode") { if (!need(v)) return false; a.overrides["stim_mode"] = v; }
        else if (k == "--stim-gain") { if (!need(v)) return false; a.overrides["stim_gain"] = v; }
        else if (k == "--stim-shape") { if (!need(v)) return false; a.stim_shape = stimulus_shape_from_name(v.c_str()); }
        else if (k == "--smooth") { if (!need(v)) return false; a.smooth_tau = std::stod(v); }
        else if (k == "--keys") {
            if (!need(v)) return false;
            std::size_t pos = 0;
            while (pos < v.size()) {
                std::size_t comma = v.find(',', pos);
                if (comma == std::string::npos) comma = v.size();
                std::string item = v.substr(pos, comma - pos);
                std::size_t colon = item.find(':');
                if (colon == std::string::npos) return false;
                a.keys.emplace_back(std::stol(item.substr(0, colon)), item.substr(colon + 1));
                pos = comma + 1;
            }
        }
        else { std::fprintf(stderr, "unknown option: %s\n", k.c_str()); return false; }
    }
    return true;
}

void usage() {
    std::printf("usage: cave_window [--model M --preset P --init I --seed N --width W --height H --scale S --sps N]\n"
                "                   [--frames N --keys \"f:key,...\" --shot-dir DIR --shot-every K --report SEC]\n"
                "                   [--wav FILE --stim-mode growth|mu --stim-gain G --stim-shape uniform|gradient_x --smooth TAU]\n"
                "keys: space pause/resume, n step, r reset, s screenshot, +/- speed, q quit\n");
}

void mkdir_p(const std::string& path) {
    std::string cur;
    for (std::size_t i = 0; i < path.size(); ++i) {
        cur += path[i];
        if (path[i] == '/' || i + 1 == path.size()) ::mkdir(cur.c_str(), 0755);
    }
}

SDL_Keycode keycode_from_name(const std::string& n) {
    if (n == "space") return SDLK_SPACE;
    if (n == "n") return SDLK_N;
    if (n == "r") return SDLK_R;
    if (n == "s") return SDLK_S;
    if (n == "q") return SDLK_Q;
    if (n == "plus") return SDLK_PLUS;
    if (n == "minus") return SDLK_MINUS;
    return SDLK_UNKNOWN;
}

bool save_screenshot(SDL_Renderer* ren, const std::string& path) {
    SDL_Surface* raw = SDL_RenderReadPixels(ren, nullptr);
    if (!raw) { std::fprintf(stderr, "SDL_RenderReadPixels: %s\n", SDL_GetError()); return false; }
    SDL_Surface* rgb = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGB24);
    SDL_DestroySurface(raw);
    if (!rgb) { std::fprintf(stderr, "SDL_ConvertSurface: %s\n", SDL_GetError()); return false; }
    std::vector<std::uint8_t> px(static_cast<std::size_t>(rgb->w) * rgb->h * 3);
    for (int y = 0; y < rgb->h; ++y) {
        const std::uint8_t* row = static_cast<const std::uint8_t*>(rgb->pixels) + static_cast<std::size_t>(y) * rgb->pitch;
        std::copy(row, row + static_cast<std::size_t>(rgb->w) * 3, px.begin() + static_cast<std::ptrdiff_t>(y) * rgb->w * 3);
    }
    const bool ok = write_png_rgb(path, rgb->w, rgb->h, px);
    SDL_DestroySurface(rgb);
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    Args a;
    if (!parse(argc, argv, a)) { usage(); return 2; }

    std::unique_ptr<Model> model;
    auto make = [&]() {
        model = make_model(a.model, a.preset, a.width, a.height, Boundary::Periodic, a.overrides);
        apply_init(*model, a.init, a.seed);
    };
    try { make(); } catch (const std::exception& e) { std::fprintf(stderr, "error: %s\n", e.what()); return 1; }
    mkdir_p(a.shot_dir);

    // --- audio + envelope (Phase 2) ---
    WavData wav;
    std::unique_ptr<EnvelopeSource> envelope;
    if (!a.wav_path.empty()) {
        std::string err;
        if (!read_wav(a.wav_path, wav, &err)) { std::fprintf(stderr, "wav: %s: %s\n", a.wav_path.c_str(), err.c_str()); return 1; }
        std::vector<float> raw = rms_envelope(wav.mono, wav.sample_rate, a.sps);
        const float peak = normalize_peak(raw);
        std::vector<float> sm = smooth_ema(raw, a.sps, a.smooth_tau);
        std::printf("wav %s: %d Hz, %zu frames, %.2f s, peak rms %.4f -> 1.0, ema tau %.3f s, envelope at %.0f/s\n",
                    a.wav_path.c_str(), wav.sample_rate, wav.mono.size(), static_cast<double>(wav.mono.size()) / wav.sample_rate, peak, a.smooth_tau, a.sps);
        envelope = std::make_unique<EnvelopeSource>(std::move(raw), std::move(sm), a.sps, a.wav_path);
    }
    const SDL_InitFlags init_flags = SDL_INIT_VIDEO | (envelope ? SDL_INIT_AUDIO : 0);
    if (!SDL_Init(init_flags)) { std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    SDL_AudioStream* audio = nullptr;
    std::size_t audio_bytes_total = 0;
    if (envelope) {
        SDL_AudioSpec spec{};
        spec.format = SDL_AUDIO_F32;
        spec.channels = 1;
        spec.freq = wav.sample_rate;
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (!audio) { std::fprintf(stderr, "SDL_OpenAudioDeviceStream: %s\n", SDL_GetError()); return 1; }
        audio_bytes_total = wav.mono.size() * sizeof(float);
        SDL_PutAudioStreamData(audio, wav.mono.data(), static_cast<int>(audio_bytes_total));
        std::printf("audio driver: %s\n", SDL_GetCurrentAudioDriver());
    }
    // Seconds of audio played so far, from the device clock. Master clock when --wav is given.
    auto audio_seconds = [&]() -> double {
        const int queued = SDL_GetAudioStreamQueued(audio);
        const std::size_t played = audio_bytes_total - static_cast<std::size_t>(std::max(0, queued));
        return static_cast<double>(played) / (sizeof(float) * static_cast<double>(wav.sample_rate));
    };
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    if (!SDL_CreateWindowAndRenderer("cave", a.width * a.scale, a.height * a.scale, 0, &win, &ren)) {
        std::fprintf(stderr, "SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, a.width, a.height);
    if (!tex) { std::fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); return 1; }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);  // crisp cells, no blur
    std::printf("SDL %d.%d.%d  video driver: %s  renderer: %s  window %dx%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
                SDL_GetCurrentVideoDriver(), SDL_GetRendererName(ren), a.width * a.scale, a.height * a.scale);
    {
        const SDL_PropertiesID props = SDL_GetWindowProperties(win);
        const Sint64 xid = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (xid) std::printf("X11 window id: 0x%llx  flags: 0x%llx\n", static_cast<unsigned long long>(xid), static_cast<unsigned long long>(SDL_GetWindowFlags(win)));
    }

    bool running = true, paused = false;
    if (audio) SDL_ResumeAudioStreamDevice(audio);  // start playing with the simulation
    unsigned long long steps_at_reset = 0;         // audio-driven mode: step count when playback started
    double accumulator = 0.0;  // seconds of simulation time owed
    const int max_steps_per_frame = 64;  // cap catch-up so a stall does not spiral
    Uint64 last_ns = SDL_GetTicksNS();
    double report_acc = 0.0;
    long frame = 0;
    std::size_t key_idx = 0;
    int shot_count = 0;

    auto shot = [&]() {
        char name[64];
        std::snprintf(name, sizeof(name), "/shot_%04d_step%08llu.png", shot_count++, static_cast<unsigned long long>(model->step_count()));
        const std::string path = a.shot_dir + name;
        std::printf("%s screenshot -> %s\n", save_screenshot(ren, path) ? "saved" : "FAILED", path.c_str());
    };
    auto handle_key = [&](SDL_Keycode key) {
        switch (key) {
            case SDLK_SPACE: paused = !paused; accumulator = 0.0; if (audio) { if (paused) SDL_PauseAudioStreamDevice(audio); else SDL_ResumeAudioStreamDevice(audio); } std::printf("%s at step %llu\n", paused ? "PAUSED" : "RESUMED", static_cast<unsigned long long>(model->step_count())); break;
            case SDLK_N: if (paused) { model->step(); std::printf("step -> %llu\n", static_cast<unsigned long long>(model->step_count())); } break;
            case SDLK_R: make(); accumulator = 0.0; if (audio) { SDL_ClearAudioStream(audio); SDL_PutAudioStreamData(audio, wav.mono.data(), static_cast<int>(audio_bytes_total)); steps_at_reset = 0; } std::printf("RESET (seed %u)\n", a.seed); break;
            case SDLK_S: shot(); break;
            case SDLK_PLUS: case SDLK_EQUALS: case SDLK_KP_PLUS: a.sps *= 2.0; std::printf("steps/s = %.1f\n", a.sps); break;
            case SDLK_MINUS: case SDLK_KP_MINUS: a.sps = std::max(1.0, a.sps / 2.0); std::printf("steps/s = %.1f\n", a.sps); break;
            case SDLK_Q: case SDLK_ESCAPE: running = false; break;
            default: break;
        }
    };

    while (running) {
        // --- scripted keys for headless verification ---
        while (key_idx < a.keys.size() && a.keys[key_idx].first == frame) {
            SDL_Event ev{};
            ev.type = SDL_EVENT_KEY_DOWN;
            ev.key.key = keycode_from_name(a.keys[key_idx].second);
            ev.key.down = true;
            SDL_PushEvent(&ev);
            ++key_idx;
        }
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            else if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) handle_key(e.key.key);
        }

        // --- fixed-step update decoupled from frame rate ---
        const Uint64 now_ns = SDL_GetTicksNS();
        const double frame_dt = static_cast<double>(now_ns - last_ns) * 1e-9;
        last_ns = now_ns;
        if (!paused) {
            auto do_step = [&]() {
                Stimulus stim;
                stim.shape = a.stim_shape;
                if (envelope) stim.amplitude = envelope->amplitude(static_cast<double>(model->step_count()) / a.sps);
                model->step(stim);
            };
            if (audio) {
                // audio clock is the master: run the steps that the played audio time calls for
                const unsigned long long target = steps_at_reset + static_cast<unsigned long long>(audio_seconds() * a.sps);
                int n = 0;
                while (model->step_count() < target && n < max_steps_per_frame) { do_step(); ++n; }
            } else {
                accumulator += frame_dt;
                const double step_period = 1.0 / a.sps;
                int n = 0;
                while (accumulator >= step_period && n < max_steps_per_frame) {
                    do_step();
                    accumulator -= step_period;
                    ++n;
                }
                if (n == max_steps_per_frame) accumulator = 0.0;  // drop the backlog instead of chasing it
            }
        }

        // --- draw ---
        const Grid& g = *model->channels()[0].grid;
        const std::vector<std::uint8_t> rgb = render_rgb(g, 1, a.colormap, 0.0f, a.vmax);
        SDL_UpdateTexture(tex, nullptr, rgb.data(), a.width * 3);
        SDL_RenderClear(ren);
        SDL_RenderTexture(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);

        // --- periodic report to the terminal and window title ---
        report_acc += frame_dt;
        if (report_acc >= a.report_seconds) {
            report_acc = 0.0;
            const ShapeMetrics sh = compute_shape(g, model->boundary(), a.threshold);
            char title[160];
            const float amp_now = envelope ? envelope->amplitude(static_cast<double>(model->step_count()) / a.sps) : 0.0f;
            std::snprintf(title, sizeof(title), "cave  %s  step %llu  t=%.1f  %.0f steps/s  mass %.1f  comp %zu  stim %.2f%s", paused ? "PAUSED" : "RUN",
                          static_cast<unsigned long long>(model->step_count()), model->time(), a.sps, sh.mass, sh.components, amp_now,
                          audio ? "  (audio clock)" : "");
            SDL_SetWindowTitle(win, title);
            std::printf("frame %ld  %s\n", frame, title);
        }
        if (a.shot_every > 0 && frame % a.shot_every == 0) shot();

        ++frame;
        if (a.frames >= 0 && frame >= a.frames) running = false;
        SDL_Delay(1);
    }

    if (audio) SDL_DestroyAudioStream(audio);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
