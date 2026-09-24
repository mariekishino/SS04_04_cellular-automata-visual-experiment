// cave: Phase 0 command-line runner.
//
// Runs one model from a preset + initial condition for N steps, writing
//   DIR/config.json   everything needed to reproduce (docs/02)
//   DIR/metrics.csv   one row per snapshot
//   DIR/frames/*.png  one image per snapshot
//   DIR/state.bin     checkpoint at the end (for --resume)
//   DIR/summary.txt   health flags and timing
//
// Stop / resume / reset in headless form:
//   stop   = the run ends at --steps and writes state.bin
//   resume = --resume continues from DIR/state.bin with the same flags
//   reset  = run again without --resume (same seed => same result)

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>

#include "core/input.hpp"
#include "core/metrics.hpp"
#include "core/model.hpp"
#include "core/presets.hpp"
#include "io/colormap.hpp"
#include "io/png.hpp"
#include "io/wav.hpp"

#ifndef CAVE_GIT_HASH
#define CAVE_GIT_HASH "unknown"
#endif

using namespace cave;

namespace {

struct Args {
    std::string model = "lenia";
    std::string preset = "orbium";
    std::string init = "orbium";
    unsigned seed = 1;
    int width = 64, height = 64;
    long steps = 200;
    long every = 10;
    std::string out = "experiments/out/run";
    Boundary boundary = Boundary::Periodic;
    Overrides overrides;
    float threshold = 0.1f;
    int scale = 4;
    Colormap colormap = Colormap::Viridis;
    float vmax = -1.0f;  // <0 means model default
    bool resume = false;
    bool list = false;
    bool quiet = false;
    // --- Phase 2: stimulus ---
    std::string stim = "none";          // none | pulse | wav
    StimulusShape stim_shape = StimulusShape::Uniform;
    PulseParams pulse;
    std::string wav_path;
    double smooth_tau = 0.05;           // seconds, EMA on the RMS envelope
    double sps = 60.0;                  // simulation steps per real second (time base for the input)
};

void usage() {
    std::printf(
        "usage: cave [options]\n"
        "  --model lenia|grayscott   --preset NAME   --init NAME   --seed N\n"
        "  --width W --height H      --steps N       --every K (snapshot interval)\n"
        "  --out DIR                 --boundary periodic|fixed\n"
        "  --set key=value           (repeatable; lenia: R mu sigma dt / grayscott: Du Dv F k dt)\n"
        "  --threshold T             (count_above threshold, default 0.1)\n"
        "  --scale S --colormap gray|viridis --vmax V\n"
        "  --resume                  (continue from DIR/state.bin)\n"
        "  --list                    (show presets and inits)\n"
        "  --quiet\n"
        "stimulus (Phase 2):\n"
        "  --stim none|pulse|wav     --stim-shape uniform|gradient_x\n"
        "  --stim-mode none|growth|mu --stim-gain G       (shortcuts for --set stim_mode= / stim_gain=)\n"
        "  --pulse-start S --pulse-dur S --pulse-period S --pulse-count N|-1 --pulse-amp A\n"
        "  --wav FILE --smooth TAU_SECONDS --sps STEPS_PER_SECOND (time base, default 60)\n");
}

bool parse(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto need = [&](std::string& v) { if (i + 1 >= argc) return false; v = argv[++i]; return true; };
        std::string v;
        if (k == "--model") { if (!need(a.model)) return false; }
        else if (k == "--preset") { if (!need(a.preset)) return false; }
        else if (k == "--init") { if (!need(a.init)) return false; }
        else if (k == "--seed") { if (!need(v)) return false; a.seed = static_cast<unsigned>(std::stoul(v)); }
        else if (k == "--width") { if (!need(v)) return false; a.width = std::stoi(v); }
        else if (k == "--height") { if (!need(v)) return false; a.height = std::stoi(v); }
        else if (k == "--steps") { if (!need(v)) return false; a.steps = std::stol(v); }
        else if (k == "--every") { if (!need(v)) return false; a.every = std::stol(v); }
        else if (k == "--out") { if (!need(a.out)) return false; }
        else if (k == "--boundary") { if (!need(v)) return false; a.boundary = (v == "fixed") ? Boundary::Fixed : Boundary::Periodic; }
        else if (k == "--set") { if (!need(v)) return false; auto p = v.find('='); if (p == std::string::npos) return false; a.overrides[v.substr(0, p)] = v.substr(p + 1); }
        else if (k == "--threshold") { if (!need(v)) return false; a.threshold = std::stof(v); }
        else if (k == "--scale") { if (!need(v)) return false; a.scale = std::stoi(v); }
        else if (k == "--colormap") { if (!need(v)) return false; a.colormap = colormap_from_name(v); }
        else if (k == "--vmax") { if (!need(v)) return false; a.vmax = std::stof(v); }
        else if (k == "--stim") { if (!need(a.stim)) return false; }
        else if (k == "--stim-shape") { if (!need(v)) return false; a.stim_shape = stimulus_shape_from_name(v.c_str()); }
        else if (k == "--stim-mode") { if (!need(v)) return false; a.overrides["stim_mode"] = v; }
        else if (k == "--stim-gain") { if (!need(v)) return false; a.overrides["stim_gain"] = v; }
        else if (k == "--pulse-start") { if (!need(v)) return false; a.pulse.start = std::stod(v); }
        else if (k == "--pulse-dur") { if (!need(v)) return false; a.pulse.duration = std::stod(v); }
        else if (k == "--pulse-period") { if (!need(v)) return false; a.pulse.period = std::stod(v); }
        else if (k == "--pulse-count") { if (!need(v)) return false; a.pulse.count = std::stoi(v); }
        else if (k == "--pulse-amp") { if (!need(v)) return false; a.pulse.amplitude = std::stof(v); }
        else if (k == "--wav") { if (!need(a.wav_path)) return false; }
        else if (k == "--smooth") { if (!need(v)) return false; a.smooth_tau = std::stod(v); }
        else if (k == "--sps") { if (!need(v)) return false; a.sps = std::stod(v); }
        else if (k == "--resume") a.resume = true;
        else if (k == "--list") a.list = true;
        else if (k == "--quiet") a.quiet = true;
        else if (k == "--help" || k == "-h") return false;
        else { std::fprintf(stderr, "unknown option: %s\n", k.c_str()); return false; }
    }
    return true;
}

void mkdir_p(const std::string& path) {
    std::string cur;
    for (std::size_t i = 0; i < path.size(); ++i) {
        cur += path[i];
        if (path[i] == '/' || i + 1 == path.size()) ::mkdir(cur.c_str(), 0755);
    }
}

std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
    return o;
}

void write_config(const Args& a, const Model& m, const std::string& path, const std::string& input_record) {
    std::ofstream f(path);
    f << "{\n";
    f << "  \"model\": \"" << m.name() << "\",\n";
    f << "  \"code_version\": \"" << CAVE_GIT_HASH << "\",\n";
    f << "  \"grid_size\": [" << a.width << ", " << a.height << "],\n";
    f << "  \"initial_state_or_seed\": {\"init\": \"" << a.init << "\", \"seed\": " << a.seed << "},\n";
    f << "  \"boundary\": \"" << boundary_name(m.boundary()) << "\",\n";
    f << "  \"timestep\": " << m.dt() << ",\n";
    f << "  \"preset\": \"" << a.preset << "\",\n";
    f << "  \"model_parameters\": {";
    bool first = true;
    for (const auto& kv : m.parameters()) {
        f << (first ? "" : ", ") << "\"" << kv.first << "\": \"" << json_escape(kv.second) << "\"";
        first = false;
    }
    f << "},\n";
    f << "  \"formula\": \"" << json_escape(m.formula()) << "\",\n";
    f << "  \"input_sequence\": \"" << json_escape(input_record) << "\",\n";
    f << "  \"stimulus\": {\"source\": \"" << a.stim << "\", \"shape\": \"" << stimulus_shape_name(a.stim_shape)
      << "\", \"steps_per_second\": " << a.sps << ", \"smooth_tau_seconds\": " << a.smooth_tau << ", \"log\": \"stimulus.csv\"},\n";
    f << "  \"duration_steps\": " << a.steps << ",\n";
    f << "  \"snapshot_every\": " << a.every << ",\n";
    f << "  \"metrics_threshold\": " << a.threshold << ",\n";
    f << "  \"render\": {\"scale\": " << a.scale << ", \"colormap\": \"" << colormap_name(a.colormap) << "\", \"vmin\": 0, \"vmax\": " << a.vmax << "},\n";
    f << "  \"resumed\": " << (a.resume ? "true" : "false") << "\n";
    f << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
    Args a;
    if (!parse(argc, argv, a)) { usage(); return 2; }
    if (a.list) {
        std::printf("presets:\n");
        for (const auto& p : list_presets()) std::printf("  %-10s %-10s %s\n", p.model.c_str(), p.name.c_str(), p.description.c_str());
        std::printf("inits:\n");
        for (const auto& p : list_inits()) std::printf("  %-10s %-10s %s\n", p.model.c_str(), p.name.c_str(), p.description.c_str());
        return 0;
    }

    std::unique_ptr<Model> model;
    try {
        model = make_model(a.model, a.preset, a.width, a.height, a.boundary, a.overrides);
        if (a.resume) {
            std::ifstream in(a.out + "/state.bin", std::ios::binary);
            if (!in) { std::fprintf(stderr, "cannot open %s/state.bin\n", a.out.c_str()); return 1; }
            model->load_state(in);
            const Grid* g = model->channels()[0].grid;
            if (g->width() != a.width || g->height() != a.height) {
                std::fprintf(stderr, "checkpoint grid %dx%d does not match --width/--height\n", g->width(), g->height());
                return 1;
            }
        } else {
            apply_init(*model, a.init, a.seed);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    if (a.vmax < 0.0f) a.vmax = (a.model == "grayscott") ? 0.5f : 1.0f;

    // --- input source (Phase 2) ---
    std::unique_ptr<InputSource> source;
    std::string input_record = "none (no stimulus)";
    float wav_peak = 0.0f;
    if (a.stim == "pulse") {
        source = std::make_unique<PulseSource>(a.pulse);
        input_record = source->describe();
    } else if (a.stim == "wav") {
        WavData w; std::string err;
        if (!read_wav(a.wav_path, w, &err)) { std::fprintf(stderr, "wav: %s: %s\n", a.wav_path.c_str(), err.c_str()); return 1; }
        std::vector<float> raw = rms_envelope(w.mono, w.sample_rate, a.sps);
        wav_peak = normalize_peak(raw);
        std::vector<float> sm = smooth_ema(raw, a.sps, a.smooth_tau);
        std::ostringstream d;
        d << "wav " << a.wav_path << " (" << w.sample_rate << " Hz, " << w.channels << " ch, " << w.bits << (w.is_float ? "-bit float" : "-bit PCM")
          << ", " << w.mono.size() << " frames) rms window 1/" << a.sps << " s, peak_rms " << wav_peak << " -> 1.0, ema tau " << a.smooth_tau << " s";
        input_record = d.str();
        source = std::make_unique<EnvelopeSource>(std::move(raw), std::move(sm), a.sps, input_record);
    } else if (a.stim != "none") {
        std::fprintf(stderr, "unknown --stim %s\n", a.stim.c_str());
        return 1;
    }

    mkdir_p(a.out + "/frames");
    write_config(a, *model, a.out + "/config.json", input_record);

    const std::vector<Channel> chans = model->channels();
    std::vector<std::string> names;
    for (const Channel& c : chans) names.push_back(c.name);

    std::ofstream csv(a.out + "/metrics.csv", a.resume ? std::ios::app : std::ios::out);
    if (!a.resume) csv << csv_header(names) << "," << names[0] << "_cx," << names[0] << "_cy," << names[0] << "_spread," << names[0] << "_components," << names[0] << "_speed," << names[0] << "_heading_deg,stim_mean\n";
    std::ofstream stim_csv(a.out + "/stimulus.csv", a.resume ? std::ios::app : std::ios::out);
    if (!a.resume) stim_csv << "step,t,t_real_seconds,raw,amplitude\n";

    std::vector<Grid> prev(chans.size());
    std::vector<std::vector<ChannelMetrics>> series(chans.size());
    const std::size_t cells = chans[0].grid->size();
    bool have_prev = false;
    ShapeMetrics prev_shape;
    unsigned long long prev_shape_step = 0;
    std::vector<double> speeds;
    // Speed = path length of the centroid / elapsed time, with the centroid tracked EVERY step.
    // Sampling only at snapshots would alias: with periodic wrap, a displacement larger than
    // W/2 between samples is folded back (observed in Phase 1 with --every 100).
    ShapeMetrics track = compute_centroid(*chans[0].grid, model->boundary());
    double path_len = 0.0, disp_x = 0.0, disp_y = 0.0;   // per-interval path length and net displacement
    double stim_sum = 0.0; unsigned long long stim_n = 0;  // mean amplitude over the interval
    std::size_t comp_min = 0, comp_max = 0;
    double spread_min = 0.0, spread_max = 0.0;

    auto snapshot = [&]() {
        std::vector<ChannelMetrics> ms;
        for (std::size_t i = 0; i < chans.size(); ++i) {
            ms.push_back(compute_metrics(*chans[i].grid, have_prev ? &prev[i] : nullptr, a.threshold));
            series[i].push_back(ms.back());
            prev[i] = *chans[i].grid;  // copy: O(cells), only at snapshots
        }
        const ShapeMetrics sh = compute_shape(*chans[0].grid, model->boundary(), a.threshold);
        double speed = 0.0;
        if (have_prev && model->step_count() > prev_shape_step) {
            const double elapsed = static_cast<double>(model->step_count() - prev_shape_step) * model->dt();
            speed = path_len / elapsed;  // cells per unit time, from per-step centroid tracking
            speeds.push_back(speed);
        }
        const double heading = (disp_x != 0.0 || disp_y != 0.0) ? std::atan2(disp_y, disp_x) * 180.0 / 3.14159265358979323846 : 0.0;  // screen coords: +x right, +y down
        const double stim_mean = stim_n ? stim_sum / static_cast<double>(stim_n) : 0.0;
        path_len = 0.0; disp_x = 0.0; disp_y = 0.0; stim_sum = 0.0; stim_n = 0;
        if (!have_prev) { comp_min = comp_max = sh.components; spread_min = spread_max = sh.spread; }
        else {
            comp_min = std::min(comp_min, sh.components); comp_max = std::max(comp_max, sh.components);
            spread_min = std::min(spread_min, sh.spread); spread_max = std::max(spread_max, sh.spread);
        }
        prev_shape = sh;
        prev_shape_step = model->step_count();
        have_prev = true;
        csv << csv_row(model->step_count(), model->time(), ms) << ',' << sh.cx << ',' << sh.cy << ',' << sh.spread << ',' << sh.components << ',' << speed << ',' << heading << ',' << stim_mean << "\n";
        char name[64];
        std::snprintf(name, sizeof(name), "/frames/frame_%07llu.png", static_cast<unsigned long long>(model->step_count()));
        write_png_rgb(a.out + name, chans[0].grid->width() * a.scale, chans[0].grid->height() * a.scale,
                      render_rgb(*chans[0].grid, a.scale, a.colormap, 0.0f, a.vmax));
        if (!a.quiet) {
            std::printf("step %8llu  %s: sum %.3f above %zu  c=(%.1f,%.1f) spread %.2f comp %zu speed %.3f head %.1f stim %.3f%s\n",
                        static_cast<unsigned long long>(model->step_count()), names[0].c_str(), ms[0].sum,
                        ms[0].count_above, sh.cx, sh.cy, sh.spread, sh.components, speed, heading, stim_mean, ms[0].finite ? "" : "  NON-FINITE");
        }
    };

    const auto t0 = std::chrono::steady_clock::now();
    const unsigned long long start = model->step_count();
    const unsigned long long target = start + static_cast<unsigned long long>(a.steps);
    snapshot();
    while (model->step_count() < target) {
        Stimulus stim;
        stim.shape = a.stim_shape;
        const double t_real = static_cast<double>(model->step_count()) / a.sps;
        float raw = 0.0f;
        if (source) { stim.amplitude = source->amplitude(t_real); raw = source->raw_amplitude(t_real); }
        stim_csv << model->step_count() << ',' << model->time() << ',' << t_real << ',' << raw << ',' << stim.amplitude << "\n";
        stim_sum += stim.amplitude; ++stim_n;
        model->step(stim);
        {
            const ShapeMetrics now = compute_centroid(*chans[0].grid, model->boundary());
            if (now.mass > 0.0 && track.mass > 0.0) {
                double dx, dy;
                displacement(track.cx, track.cy, now.cx, now.cy, chans[0].grid->width(), chans[0].grid->height(), model->boundary(), dx, dy);
                path_len += std::sqrt(dx * dx + dy * dy);
                disp_x += dx; disp_y += dy;
            }
            track = now;
        }
        if (model->step_count() % static_cast<unsigned long long>(a.every) == 0 || model->step_count() == target) snapshot();
    }
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    {
        std::ofstream st(a.out + "/state.bin", std::ios::binary);
        model->save_state(st);
    }

    HealthConfig hc;
    std::ofstream sum(a.out + "/summary.txt");
    sum << "code_version " << CAVE_GIT_HASH << "\n";
    sum << "input " << input_record << "\n";
    sum << "steps " << start << " -> " << model->step_count() << "  (" << a.steps << " this run)\n";
    sum << "wall_seconds " << secs << "  ms_per_step " << (a.steps > 0 ? 1000.0 * secs / static_cast<double>(a.steps) : 0.0) << "\n";
    sum << "grid " << a.width << "x" << a.height << "  cells " << cells << "\n";
    for (std::size_t i = 0; i < chans.size(); ++i) {
        const HealthFlags h = evaluate_health(series[i], cells, hc);
        sum << "channel " << names[i] << ": nan_or_inf=" << h.nan_or_inf << " extinct=" << h.extinct
            << " saturated=" << h.saturated << " static=" << h.static_ << "  (threshold " << a.threshold
            << ", saturation_fraction " << hc.saturation_fraction << ", static_eps " << hc.static_eps
            << " over " << hc.static_window << " snapshots)\n";
    }
    {
        double mean_speed = 0.0;
        for (double v : speeds) mean_speed += v;
        if (!speeds.empty()) mean_speed /= static_cast<double>(speeds.size());
        sum << "shape " << names[0] << ": mean_speed " << mean_speed << " (cells per unit time, centroid path length tracked every step, over " << speeds.size()
            << " intervals)  components " << comp_min << ".." << comp_max << "  spread " << spread_min << ".." << spread_max
            << "  final mass " << prev_shape.mass << " centroid (" << prev_shape.cx << ", " << prev_shape.cy << ")\n";
    }
    if (!a.quiet) {
        std::printf("done: %llu steps in %.2fs (%.3f ms/step) -> %s\n", static_cast<unsigned long long>(a.steps), secs,
                    a.steps > 0 ? 1000.0 * secs / static_cast<double>(a.steps) : 0.0, a.out.c_str());
    }
    return 0;
}
