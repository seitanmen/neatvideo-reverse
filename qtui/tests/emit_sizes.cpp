// qtui_emit_sizes - the OFX frame processing of the C++ UI (nvqt::ofx_frame + nvqt::ofx_process, as the window's
// Apply / close does it) for many frame sizes in one process (qtui/tools/compare_emit_sizes.py compares it with the
// Python UI's main._emit).  Usage: qtui_emit_sizes JOBS.txt; one job per line:
//   W H DEPTH NFRAMES T FRAMES.bin P.json OUT.bin
// FRAMES.bin holds NFRAMES frames of the OFX layout, P.json the backend parameters; frame T is processed with the Auto
// Profile of frame T (bt709, limited range).  Prints "OK <bytes>" (OUT.bin written) or "ERR <message>" per job.
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "depth.h"
#include "engine.h"
#include "ofx.h"
#include "pyobj.h"

using namespace nvqt;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: qtui_emit_sizes JOBS.txt\n");
        return 2;
    }
    std::ifstream jobs(argv[1]);
    std::string line;
    while (std::getline(jobs, line)) {
        std::istringstream is(line);
        int W, H, d, n, t;
        std::string fpath, ppath, opath;
        if (!(is >> W >> H >> d >> n >> t >> fpath >> ppath >> opath)) continue;
        std::ifstream ff(fpath, std::ios::binary), pf(ppath, std::ios::binary);
        const std::vector<uint8_t> data((std::istreambuf_iterator<char>(ff)), std::istreambuf_iterator<char>());
        const std::string ptext((std::istreambuf_iterator<char>(pf)), std::istreambuf_iterator<char>());
        const size_t fs = (static_cast<size_t>(W) * H + 2 * static_cast<size_t>(W / 2) * (H / 2)) * (d / 8);
        std::vector<Frame> frames;
        std::vector<RestoreMap> restore;
        for (int k = 0; k < n && (k + 1) * fs <= data.size(); ++k) {
            RestoreMap rm;
            frames.push_back(ofx_frame(data.data() + k * fs, W, H, d, &rm));
            restore.push_back(std::move(rm));
        }
        try {
            const PyObj P = PyObj::parse(ptext);
            PreviewEngine engine(&frames, "bt709", false);
            const RestoreMap* rs = restore[static_cast<size_t>(t)].empty() ? nullptr : &restore[static_cast<size_t>(t)];
            const std::vector<uint8_t> out = ofx_process(frames, t, P, nullptr, false, "bt709", engine, d, rs);
            std::ofstream of(opath, std::ios::binary);
            of.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
            std::printf("OK %zu\n", out.size());
        } catch (const std::exception& e) {
            std::printf("ERR %s\n", e.what());
        }
        std::fflush(stdout);
    }
    return 0;
}
