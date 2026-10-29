// export - the full-video export of the C++ UI: repro/nv_ui_qt/export.py + repro/denoise_video_v5.py main()
// (probe, the constant-rate frame reader at the processing depth (nv_depth), audio_plan, pick_codec,
// trial_encode, the ffmpeg encoder command, the frame windows and the side files) in C++, the frames
// processed through libnvcore (nvcore.h temporal / finish = backend.process_frame).  The ffmpeg command
// lines are the Python ones word for word, so an export of the same input with the same profile / parameters
// is byte-identical to `python3 -m nv_ui_qt.export` (qtui/tools/compare_export.py).
//
// The Python UI runs the export as a separate process (python -m nv_ui_qt.export) and parses its printed
// "frame k/n" lines; the C++ UI runs ExportJob in a thread of its own and gets the same lines as signals.
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "depth.h"
#include "pyobj.h"
#include "video.h"

namespace nvqt {


// denoise_video_v5.probe: the whole dict (the UI's VideoInfo plus the export's fields)
struct ProbeInfo {
    VideoInfo v;
    double v0 = 0.0;       // time of the first decoded frame
    double fs = 0.0;       // format start_time
    bool discont = false;  // TS_DISCONT demuxer
    struct Audio {
        std::string codec;
        std::optional<double> start;
    };
    std::vector<Audio> audio;
};
ProbeInfo probe_full(const std::string& path);

bool is_sequence(const std::string& dst);                             // denoise_video_v5.is_sequence
std::string sidecar_stem(const std::string& dst);                     // denoise_video_v5.sidecar_stem
// denoise_video_v5.pick_codec: "x264" / "x265" / "ffv1" / "prores" / "exr"; throws std::runtime_error with
// the Python ValueError text
std::string pick_codec(const PixFmt& fmt, const std::string& dst, const std::string& codec);
void remove_output(const std::string& dst);                           // denoise_video_v5.remove_output

// denoise_video_v5.read_frames(path, W, H, lo, hi, fps, fmt, matrix, vf) at a processing format (fmt not the legacy
// yuv420p): the internal float32 planes (nv_depth.to_internal; float frames sanitized) of frames lo..hi-1 (hi < 0: to
// the end) until on_frame returns false; vf = the extra filter ("" = none).  The UI's loading and Batch Profiler at a
// high bit depth (the 8-bit path is video.h read_frames).
void read_frames_fmt(const std::string& path, int W, int H, int lo, int hi, const std::string& fps, const PixFmt& fmt,
                     const std::string& matrix, const std::string& vf, const std::function<bool(int, Planes&&)>& on_frame);
// nv_depth.read_filter(info, fmt, matrix)
std::string read_filter(const VideoInfo& info, const PixFmt& fmt, const std::string& matrix);

struct ExportOptions {
    std::string src, dst;        // absolute paths (the UI resolves them)
    PyObj prof;                  // the UI's effective profile (= the .ui_profile.json the UI wrote)
    PyObj params;                // state->backend() (= the .ui_params.json)
    int jobs = 1, crf = 16;
    std::string audio = "copy";  // copy / aac / none
    std::string depth = "8";     // 8 / 10 / 12 / 16 / 32 / source
    std::string codec = "auto";  // auto / x264 / x265 / ffv1 / prores / exr
    std::optional<int> start, frames;
};

// The export itself (export.main + denoise_video_v5.main), synchronous: every line the Python export prints
// goes to `print` (stdout and the sys.exit messages alike, without the trailing newline); returns the exit
// code (0 ok).  `cancel` (may be null) is polled between frames: when set the ffmpeg children are killed
// and 1 is returned (the caller removes the partial output as the Python UI does).
int run_export(const ExportOptions& o, const std::function<void(const std::string&)>& print,
               const std::atomic<bool>* cancel);

// worker.ExportProcess: run_export in a thread; progress(pct, line) for every printed line (pct = 100 k / n
// for the "frame k/n" lines, -1 otherwise), finished(ok, text) with text = the last line when ok, else the
// last 6 lines.  kill() cancels (finished(false, ..) follows).
class ExportJob : public QObject {
    Q_OBJECT
public:
    explicit ExportJob(ExportOptions o, QObject* parent = nullptr);
    ~ExportJob() override;
    void start();
    void kill();
    bool running() const { return running_; }
    QStringList log;

signals:
    void progress(double pct, QString line);
    void finished(bool ok, QString text);
    void lineSignal(QString line);          // internal: a printed line (from the worker thread)
    void doneSignal(int code);              // internal: run_export returned

private:
    ExportOptions o_;
    std::atomic<bool> cancel_{false};
    std::atomic<bool> running_{false};
    class QThread* thread_ = nullptr;
};

}  // namespace nvqt
