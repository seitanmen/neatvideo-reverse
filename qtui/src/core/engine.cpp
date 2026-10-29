// engine - see engine.h.
#include "engine.h"

#include <algorithm>

#include "nvcore_c_ui.h"

namespace nvqt {

std::string temporal_sig(const PyObj& P) {
    static const char* KEYS[] = {"radius", "temporal", "threshold", "noise_level", "similarity", "pyramid_scale", "bs",
                                 "search", "t_noise"};
    PyObj t = PyObj::list();
    for (const char* k : KEYS) t.append(std::string(k) == "t_noise" && !P.has(k) ? PyObj::real(1.0) : P.get(k));
    t.append(P.get("channel_noise").get("Y"));
    return t.dump();
}

Planes clip_planes(const Planes& p) {
    Planes o = p;
    for (int c = 0; c < 3; ++c)
        for (float& v : o.v(c)) {
            if (v != v) continue;
            if (v < 0.0f) v = 0.0f;
            if (v > 255.0f) v = 255.0f;
        }
    return o;
}

Planes PreviewEngine::crop_planes(const Frame& f, const std::optional<Crop>& crop) const {
    if (!crop) return to_planes(f);
    // planes[0][y0:y0 + h, x0:x0 + w]; chroma of another shape: [y0 // 2:(y0 + h) // 2, x0 // 2:(x0 + w) // 2]
    auto sl = [](int a, int b, int n, int* lo, int* len) {
        a = std::clamp(a, 0, n);
        b = std::clamp(b, 0, n);
        *lo = a;
        *len = std::max(0, b - a);
    };
    Planes p;
    int y, h, x, w;
    sl(crop->y0, crop->y0 + crop->h, f.H, &y, &h);
    sl(crop->x0, crop->x0 + crop->w, f.W, &x, &w);
    p.H = h;
    p.W = w;
    p.Y.resize(static_cast<size_t>(h) * w);
    for (int i = 0; i < h; ++i)
        for (int j = 0; j < w; ++j) p.Y[static_cast<size_t>(i) * w + j] = f.at(0, static_cast<size_t>(y + i) * f.W + x + j);
    const int s = (f.Hc != f.H || f.Wc != f.W) ? 2 : 1;
    int cy, ch, cx, cw;
    sl(crop->y0 / s, (crop->y0 + crop->h) / s, f.Hc, &cy, &ch);
    sl(crop->x0 / s, (crop->x0 + crop->w) / s, f.Wc, &cx, &cw);
    p.Hc = ch;
    p.Wc = cw;
    for (int c = 1; c < 3; ++c) {
        auto& dst = p.v(c);
        dst.resize(static_cast<size_t>(ch) * cw);
        for (int i = 0; i < ch; ++i)
            for (int j = 0; j < cw; ++j) dst[static_cast<size_t>(i) * cw + j] = f.at(c, static_cast<size_t>(cy + i) * f.Wc + cx + j);
    }
    return p;
}

std::pair<std::vector<int>, std::vector<Planes>> PreviewEngine::window(int t, int R, const std::optional<Crop>& crop) const {
    std::vector<int> idx;
    std::vector<Planes> win;
    for (int i = t - R; i <= t + R; ++i)
        if (i >= 0 && i < static_cast<int>(frames_->size())) {
            idx.push_back(i);
            win.push_back(crop_planes((*frames_)[static_cast<size_t>(i)], crop));
        }
    return {idx, win};
}

PreviewEngine::Stage PreviewEngine::temporal_stage(int t, const PyObj& prof, long token, const PyObj& P,
                                                   const std::optional<Crop>& crop) {
    Stage st;
    const int R = static_cast<int>(P.get("radius").as_int());
    auto [idx, win] = window(t, R, crop);
    st.idx = std::move(idx);
    st.win = std::move(win);
    st.ci = static_cast<int>(std::find(st.idx.begin(), st.idx.end(), t) - st.idx.begin());
    const std::string cs = crop ? std::to_string(crop->y0) + "," + std::to_string(crop->x0) + "," + std::to_string(crop->h) +
                                      "," + std::to_string(crop->w)
                                : "None";
    const std::string tkey = std::to_string(t) + "|" + cs + "|" + std::to_string(token) + "|" + temporal_sig(P);
    const std::string fkey = std::to_string(t) + "|" + cs + "|" + P.get("bs").dump() + "|" + P.get("search").dump();
    bool hit = false;
    Flows flows;
    {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto it = temporal_.begin(); it != temporal_.end(); ++it)
            if (it->key == tkey) {
                st.tmp = it->tmp;
                st.rv = it->rv;
                hit = true;
                temporal_.splice(temporal_.end(), temporal_, it);
                break;
            }
        auto fit = std::find_if(flows_.begin(), flows_.end(), [&](const FlowEntry& e) { return e.key == fkey; });
        if (fit == flows_.end()) {
            flows_.push_back({fkey, {}});
            fit = std::prev(flows_.end());
        } else {
            flows_.splice(flows_.end(), flows_, fit);
            fit = std::prev(flows_.end());
        }
        flows = fit->flows;
        while (flows_.size() > 16) flows_.pop_front();
    }
    if (!hit) {
        temporal(st.win, st.ci, prof, P, &flows, &st.tmp, &st.rv);
        std::lock_guard<std::mutex> lk(mu_);
        auto fit = std::find_if(flows_.begin(), flows_.end(), [&](const FlowEntry& e) { return e.key == fkey; });
        if (fit != flows_.end())
            for (auto& kv : flows) fit->flows.insert(kv);
        temporal_.push_back({tkey, st.tmp, st.rv});
        while (temporal_.size() > 6) temporal_.pop_front();
    }
    return st;
}

std::pair<std::vector<float>, std::vector<float>> PreviewEngine::process(int t, const PyObj& prof, long token,
                                                                         const PyObj& P, const std::optional<Crop>& crop) {
    const Stage st = temporal_stage(t, prof, token, P, crop);
    const Planes out = finish(st.win[static_cast<size_t>(st.ci)], st.tmp, st.rv, prof, P);
    return {to_rgb(clip_planes(out)), to_rgb(st.win[static_cast<size_t>(st.ci)])};
}

std::vector<float> PreviewEngine::mask(int t, const PyObj& prof, long token, const PyObj& P) {
    const Stage st = temporal_stage(t, prof, token, P, std::nullopt);
    std::vector<float> m(st.rv.Y.size());
    nvc_temporal_mask(st.rv.Y.data(), st.rv.H, st.rv.W, static_cast<int>(st.win.size()), P.get("temporal").as_double(),
                      m.data());
    return m;
}

std::vector<float> PreviewEngine::assist(int t, const PyObj& prof, const PyObj& P) {
    const Planes planes = crop_planes((*frames_)[static_cast<size_t>(t)], std::nullopt);
    Planes ones = planes;
    for (int c = 0; c < 3; ++c) std::fill(ones.v(c).begin(), ones.v(c).end(), 1.0f);
    const Planes out = finish(planes, planes, ones, prof, P);
    return to_rgb(clip_planes(out));
}

}  // namespace nvqt
