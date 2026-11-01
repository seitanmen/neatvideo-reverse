// nvcore_c_match - see nvcore_c_match.h.  The formula is the repro definition of N4e; every part it calls is a
// verified port (nv_model, nv_curve, nv_aft).  Exceptions do not cross the ABI.
#include "nvcore_c_match.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "nv_aft.h"
#include "nv_curve.h"
#include "nv_model.h"

namespace {

inline float bits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

const float CLIP = bits(0x3ea8f5c3u);    // 0.33f (nv_aft CLIP)
const float HALF = bits(0x3f000000u);    // 0.5f
const float S_LVL = bits(0x40400000u);   // 3.0f (nv_aft S_LVL[0])
const float S_MULT = bits(0x40800000u);  // 4.0f (nv_aft S_MULT[0])
const int FACE[3] = {0x4c, 0x1fa0, 0x3ef4};
const int STATS[3] = {0xbc44, 0xbd10, 0xbddc};

// q_c of the header for one face
float face_term(const float* m, const float* F, const float* S) {
    const float sim = nva::similarity(m, F, S, S_LVL, S_MULT);
    const float l5 = nva::penalty(std::fabs(F[5] / m[5]), S_LVL);
    const float l6 = nva::penalty(std::fabs(F[6] / m[6]), S_LVL);
    return sim * (l5 * l6);
}

// p of the header for one model (faces / stats R, G, B consecutive)
float tile_product(const float* mR, const float* mG, const float* mB, const float* F, const float* S) {
    const float qR = face_term(mR, F, S);
    const float qG = face_term(mG, F + NVC_MATCH_FACE, S + NVC_MATCH_STATS);
    const float qB = face_term(mB, F + 2 * NVC_MATCH_FACE, S + 2 * NVC_MATCH_STATS);
    const float p = (qG * qR) * qB;
    return std::isnan(p) ? 0.0f : p;
}

}  // namespace

extern "C" {

void nvc_match_model(const uint8_t* prof, float* face, float* stats) {
    for (int c = 0; c < 3; ++c) {
        std::memcpy(face + c * NVC_MATCH_FACE, prof + FACE[c], NVC_MATCH_FACE * 4);
        std::memcpy(stats + c * NVC_MATCH_STATS, prof + STATS[c], NVC_MATCH_STATS * 4);
    }
}

int nvc_match_score(const float* rgb, int W, int H, const float* ref_face, const float* ref_stats, int n,
                    const float* faces, const float* stats, int threads, nvc_cancel_fn cancel, double* scores,
                    int* accept, int* info) {
    try {
        if (!rgb || W < 1 || H < 1 || !ref_face || !ref_stats || n < 0 ||
            (n > 0 && (!faces || !stats || !scores || !accept)) || !info)
            return -2;
        for (int k = 0; k < n; ++k) {
            scores[k] = 0.0;
            accept[k] = 0;
        }
        info[0] = info[1] = info[2] = 0;
        if (W < 48 || H < 48) return 1;
        const int nx = W / 32, ny = H / 32, nt = nx * ny;
        info[0] = nt;
        // per tile: 0 clipped, 1 used, 2 model error; the products of the used tiles (nt x (1 + n): reference first)
        const size_t np = static_cast<size_t>(n) + 1;
        std::vector<char> state(static_cast<size_t>(nt), 0);
        std::vector<float> prod(static_cast<size_t>(nt) * np, 0.0f);
        std::atomic<int> next{0};
        std::atomic<bool> stop{false};
        std::mutex mu;                        // one cancel query at a time
        auto work = [&] {
            std::vector<float> R(1024), G(1024), B(1024);
            std::vector<uint32_t> wR(nva::NWORDS), wG(nva::NWORDS), wB(nva::NWORDS);
            for (;;) {
                if (stop.load()) return;
                const int s = next.fetch_add(1);
                if (s >= nt) return;
                if (cancel) {
                    std::lock_guard<std::mutex> lk(mu);
                    if (cancel() != 0) {
                        stop = true;
                        return;
                    }
                }
                const int y = 32 * (s / nx), x = 32 * (s % nx);
                for (int i = 0; i < 32; ++i)
                    for (int j = 0; j < 32; ++j) {
                        const float* px = rgb + (static_cast<size_t>(y + i) * W + (x + j)) * 3;
                        R[i * 32 + j] = px[0];
                        G[i * 32 + j] = px[1];
                        B[i * 32 + j] = px[2];
                    }
                if (nvc::is_clip_genuine(R.data(), 1024) > CLIP || nvc::is_clip_genuine(G.data(), 1024) > CLIP ||
                    nvc::is_clip_genuine(B.data(), 1024) > CLIP)
                    continue;
                if (nvm::model_full(R.data(), 32, 32, 2, wR.data(), 2) != nvm::OK ||
                    nvm::model_full(G.data(), 32, 32, 2, wG.data(), 2) != nvm::OK ||
                    nvm::model_full(B.data(), 32, 32, 2, wB.data(), 2) != nvm::OK) {
                    state[static_cast<size_t>(s)] = 2;
                    continue;
                }
                const float* mR = reinterpret_cast<const float*>(wR.data());
                const float* mG = reinterpret_cast<const float*>(wG.data());
                const float* mB = reinterpret_cast<const float*>(wB.data());
                float* ps = prod.data() + static_cast<size_t>(s) * np;
                ps[0] = tile_product(mR, mG, mB, ref_face, ref_stats);
                for (int k = 0; k < n; ++k)
                    ps[1 + k] = tile_product(mR, mG, mB, faces + static_cast<size_t>(k) * 3 * NVC_MATCH_FACE,
                                             stats + static_cast<size_t>(k) * 3 * NVC_MATCH_STATS);
                state[static_cast<size_t>(s)] = 1;
            }
        };
        const int nth = threads < 1 ? 1 : threads;
        if (nth == 1) {
            work();
        } else {
            std::vector<std::thread> pool;
            for (int t = 0; t < nth; ++t) pool.emplace_back(work);
            for (auto& th : pool) th.join();
        }
        if (stop.load()) return 2;
        int used = 0;
        for (int s = 0; s < nt; ++s) {
            if (state[static_cast<size_t>(s)] == 2) return 3;
            if (state[static_cast<size_t>(s)] == 1) ++used;
        }
        info[1] = used;
        if (used == 0) return 4;
        for (int s = 0; s < nt; ++s)
            if (state[static_cast<size_t>(s)] == 1 && prod[static_cast<size_t>(s) * np] > HALF) ++info[2];
        for (int k = 0; k < n; ++k) {
            double num = 0.0, den = 0.0;
            int acc = 0;
            for (int s = 0; s < nt; ++s) {
                if (state[static_cast<size_t>(s)] != 1) continue;
                const float r = prod[static_cast<size_t>(s) * np];
                const float p = prod[static_cast<size_t>(s) * np + 1 + static_cast<size_t>(k)];
                num += static_cast<double>(std::min(p, r));
                den += static_cast<double>(std::max(p, r));
                if (p > HALF) ++acc;
            }
            scores[k] = den > 0.0 ? num / den : 0.0;
            accept[k] = acc;
        }
        return 0;
    } catch (...) {
        return -1;
    }
}

}  // extern "C"
