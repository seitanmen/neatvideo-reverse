// viewer - see viewer.h (port of repro/nv_ui_qt/widgets/viewer.py).
#include "viewer.h"

#include <QActionGroup>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QResizeEvent>
#include <QThreadPool>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "nvcore_c_ui.h"

namespace nvqt {

const std::vector<double> ZOOMS = {5, 10, 12.5, 16.7, 20, 25, 33.3, 50, 100, 200, 300, 400, 800};
const std::vector<std::pair<std::string, std::string>> MODES = {
    {"Normal", "Normal"},
    {"YCrCb +", "YCrCb + Enhanced Contrast (Default)"},
    {"RGB", "RGB  (Red, Green, Blue)"},
    {"YCrCb", "YCrCb  (Luminance, Chrominance)"},
    {"Y Freq", "Y Channel Frequencies"},
    {"Cr Freq", "Cr Channel Frequencies"},
    {"Cb Freq", "Cb Channel Frequencies"}};
const std::map<int, std::string> MODE_SECTIONS = {{0, "Primary Modes"}, {2, "Channel Components"}, {4, "Frequency Components"}};
const std::vector<std::string> SPLITS = {"2 x 2", "1+3 Bottom", "1+3 Right"};

namespace {

const std::map<std::string, std::vector<std::string>> PANES = {
    {"Normal", {}}, {"YCrCb +", {"Y+", "Cr+", "Cb+"}}, {"RGB", {"R", "G", "B"}}, {"YCrCb", {"Y", "Cr", "Cb"}},
    {"Y Freq", {"Y High", "Y Mid", "Y Low"}}, {"Cr Freq", {"Cr High", "Cr Mid", "Cr Low"}},
    {"Cb Freq", {"Cb High", "Cb Mid", "Cb Low"}}};
const std::map<std::string, std::string> PANE_LABEL = {{"Y+", "Y Enhanced"}, {"Cr+", "Cr Enhanced"}, {"Cb+", "Cb Enhanced"}};
// display.freq_images kinds: 0 "Y+", 1 "Y", 2 "Cr", 3 "Cb"
const std::map<std::string, int> FREQ_KIND = {{"Y+", 0}, {"Cr+", 0}, {"Cb+", 0}, {"Y High", 1}, {"Y Mid", 1},
                                              {"Y Low", 1}, {"Cr High", 2}, {"Cr Mid", 2}, {"Cr Low", 2},
                                              {"Cb High", 3}, {"Cb Mid", 3}, {"Cb Low", 3}};
const QColor MAIN_IN(98, 171, 255), MAIN_OUT(32, 32, 140);
const QColor PREVIEW_IN(15, 15, 255), PREVIEW_OUT(0, 0, 110);
const QColor PANE_SEL_IN(192, 192, 192), PANE_SEL_OUT(64, 64, 64);
const QColor WARN(255, 208, 15);
const QColor PCHECK_TEXT(240, 208, 0);

}  // namespace

void Viewer::sel_colors(int w, int h, QColor* inner, QColor* outer) {
    const int m = std::min(w, h);
    if (m <= 15) {
        *inner = QColor(255, 15, 15);
        *outer = QColor(128, 0, 0);
    } else if (m <= 127) {
        *inner = QColor(255, 208, 15);
        *outer = QColor(100, 97, 2);
    } else {
        *inner = QColor(15, 192, 15);
        *outer = QColor(0, 94, 0);
    }
}

namespace {

// display.pane_kind -> output-range kind of nvcore_c_ui (RANGES order Y, R, G, B, Cr, Cb)
int range_kind(const std::string& name) {
    std::string k = name.substr(0, name.find(' '));
    if (!k.empty() && k.back() == '+') k.pop_back();
    static const std::map<std::string, int> RK = {{"Y", 0}, {"R", 1}, {"G", 2}, {"B", 3}, {"Cr", 4}, {"Cb", 5}};
    return RK.at(k);
}

int simple_name(const std::string& name) {
    static const std::map<std::string, int> S = {{"rgb", 0}, {"R", 1}, {"G", 2}, {"B", 3}, {"Y", 4}, {"Cr", 5}, {"Cb", 6}};
    return S.at(name);
}

std::string fmt(const char* f, double v) {
    char b[64];
    std::snprintf(b, sizeof(b), f, v);
    return b;
}

double py_round(double x) { return std::nearbyint(x); }   // Python round(x): half to even

}  // namespace

RgbImage rot90(const std::vector<float>& rgb, int H, int W, int k) {
    k = ((k % 4) + 4) % 4;
    RgbImage o;
    if (k % 2) {
        o.H = W;
        o.W = H;
    } else {
        o.H = H;
        o.W = W;
    }
    o.d.resize(rgb.size());
    if (k == 0) {
        o.d = rgb;
        return o;
    }
    for (int i = 0; i < o.H; ++i)
        for (int j = 0; j < o.W; ++j) {
            int y, x;
            if (k == 1) {
                y = j;
                x = W - 1 - i;
            } else if (k == 2) {
                y = H - 1 - i;
                x = W - 1 - j;
            } else {
                y = H - 1 - j;
                x = i;
            }
            std::memcpy(&o.d[(static_cast<size_t>(i) * o.W + j) * 3], &rgb[(static_cast<size_t>(y) * W + x) * 3], 12);
        }
    return o;
}

QImage to_qimage(const std::vector<uint8_t>& rgb, int H, int W) {
    return QImage(rgb.data(), W, H, 3 * W, QImage::Format_RGB888).copy();
}

QImage to_qimage_f(const std::vector<float>& rgb, int H, int W) {
    // np.clip(rgb, 0, 255).astype(np.uint8) (truncation)
    std::vector<uint8_t> u(rgb.size());
    for (size_t i = 0; i < rgb.size(); ++i) {
        float v = rgb[i];
        if (!(v >= 0.0f)) v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        u[i] = static_cast<uint8_t>(v);
    }
    return to_qimage(u, H, W);
}

// =================================================================== Viewer
Viewer::Viewer(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    pool_ = new QThreadPool(this);
    pool_->setMaxThreadCount(1);                       // ThreadPoolExecutor(max_workers=1)
    connect(this, &Viewer::computedSignal, this, &Viewer::on_computed, Qt::QueuedConnection);
}

Viewer::~Viewer() {
    pool_->clear();
    pool_->waitForDone();
}

void Viewer::set_mode(const std::string& m) {
    if (PANES.count(m) && m != mode_) {
        mode_ = m;
        update();
        emit viewChanged();
    }
}

void Viewer::set_split(const std::string& s) {
    if (std::find(SPLITS.begin(), SPLITS.end(), s) != SPLITS.end() && s != split_) {
        split_ = s;
        update();
        emit viewChanged();
    }
}

void Viewer::set_zoom(double z) {
    if (z != zoom_) {
        zoom_ = z;
        if (z == 0) center_.reset();
        update();
        emit viewChanged();
    }
}

void Viewer::zoom_step(int d) {
    const double s = scale() * 100.0;
    if (d > 0) {
        for (double z : ZOOMS)
            if (z > s * 1.001) {
                set_zoom(z);
                return;
            }
        set_zoom(ZOOMS.back());
    } else {
        for (auto it = ZOOMS.rbegin(); it != ZOOMS.rend(); ++it)
            if (*it < s * 0.999) {
                set_zoom(*it);
                return;
            }
        set_zoom(ZOOMS.front());
    }
}

void Viewer::set_brightness(int b) {
    brightness_ = std::min(std::max(b, 0), 100);
    cache_.clear();
    update();
}

void Viewer::set_emphasis(bool on) {
    if (on != emphasis_) {
        emphasis_ = on;
        cache_.clear();
        update();
    }
}

void Viewer::set_show_filtered(bool on) {
    if (on != show_filtered_) {
        show_filtered_ = on;
        update();
    }
}

void Viewer::set_assist(bool on) {
    assist_ = on;
    cache_.clear();
    update();
}

bool Viewer::showing_filtered() const { return show_filtered_ && has_filt_ && !hold_; }

void Viewer::set_pcheck(bool on, const std::vector<float>* img) {
    pcheck_ = on;
    if (!on) {
        f_.erase("pcheck");
    } else if (img) {
        RgbImage fo = rot90(*img, cH_, cW_, rot_);
        auto it = f_.find("pcheck");
        if (it == f_.end() || it->second.H != fo.H || it->second.W != fo.W || it->second.d != fo.d) {
            f_["pcheck"] = std::move(fo);
            tok_["pcheck"] = ++ntok_;
        }
    }
    drop_freq();
    cache_.clear();
    update();
}

bool Viewer::showing_pcheck() const { return pcheck_ && f_.count("pcheck"); }

void Viewer::set_nlc(bool on, const std::vector<float>* img, bool keep) {
    nlc_ = on;
    if (!on || (!img && !keep)) {
        f_.erase("nlc");
    } else if (img) {
        RgbImage fo = rot90(*img, cH_, cW_, rot_);
        auto it = f_.find("nlc");
        if (it == f_.end() || it->second.H != fo.H || it->second.W != fo.W || it->second.d != fo.d) {
            f_["nlc"] = std::move(fo);
            tok_["nlc"] = ++ntok_;
        }
    }
    drop_freq();
    cache_.clear();
    update();
}

bool Viewer::showing_nlc() const { return nlc_ && f_.count("nlc") && !hold_; }

void Viewer::drop_freq() {
    std::set<long> live;
    for (const auto& kv : tok_)
        if (f_.count(kv.first)) live.insert(kv.second);
    for (auto it = freq_.begin(); it != freq_.end();) {
        const long t = std::stol(it->first.substr(0, it->first.find('|')));
        it = live.count(t) ? std::next(it) : freq_.erase(it);
    }
}

void Viewer::set_selection(std::optional<Rect> rect, const std::string& style, const QString& label, const QStringList& warn) {
    sel_ = rect;
    sel_style_ = style;
    sel_label_ = label;
    sel_warn_ = warn;
    update();
}

void Viewer::set_main_sample(std::optional<Rect> rect) {
    main_sample_ = rect;
    update();
}

void Viewer::set_images(const std::vector<float>& orig, int H, int W, const std::vector<float>* filt,
                        std::optional<Rect> region, int rot) {
    rot = ((rot % 4) + 4) % 4;
    std::vector<float> full;
    const std::vector<float>* fp = filt;
    if (filt && region) {
        full = orig;
        const Rect& r = *region;
        for (int y = 0; y < r.h; ++y)
            std::memcpy(&full[(static_cast<size_t>(r.y0 + y) * W + r.x0) * 3], &(*filt)[static_cast<size_t>(y) * r.w * 3],
                        sizeof(float) * 3 * static_cast<size_t>(r.w));
        fp = &full;
    }
    RgbImage o = rot90(orig, H, W, rot);
    auto it = f_.find("orig");
    const bool new_orig = it == f_.end() || rot_ != rot || it->second.H != o.H || it->second.W != o.W || it->second.d != o.d;
    rot_ = rot;
    cH_ = H;
    cW_ = W;
    region_ = region;
    if (new_orig) {
        dH_ = o.H;
        dW_ = o.W;
        f_["orig"] = std::move(o);
        has_orig_ = true;
        tok_["orig"] = ++ntok_;
    }
    if (!fp) {
        f_.erase("filt");
        has_filt_ = false;
    } else {
        RgbImage fo = rot90(*fp, H, W, rot);
        auto ft = f_.find("filt");
        if (ft == f_.end() || ft->second.H != fo.H || ft->second.W != fo.W || ft->second.d != fo.d) {
            f_["filt"] = std::move(fo);
            tok_["filt"] = ++ntok_;
        }
        has_filt_ = true;
    }
    drop_freq();
    cache_.clear();
    update();
    if (new_orig && zoom_ == 0) emit viewChanged();
}

std::string Viewer::source() const {
    if (showing_pcheck()) return "pcheck";
    if (showing_nlc()) return "nlc";
    if (!showing_filtered()) return "orig";
    return assist_ ? "assist" : "filt";
}

std::string Viewer::plane_src(const std::string& src) const {
    return (src == "orig" || src == "pcheck" || src == "nlc") ? src : "filt";
}

const RgbImage& Viewer::img(const std::string& src) const { return f_.at(plane_src(src)); }

std::shared_ptr<FreqImages> Viewer::freq_get(const std::string& src, int kind, bool* failed) {
    const std::string s = plane_src(src);
    const bool emph = kind == 0 ? (emphasis_ && s != "pcheck" && s != "nlc") : false;
    const std::string key = std::to_string(tok_[s]) + "|" + std::to_string(kind) + "|" + (emph ? "1" : "0");
    if (failed) *failed = false;
    auto it = freq_.find(key);
    if (it != freq_.end()) {
        if (failed) *failed = !it->second.pending && !it->second.fi;
        return it->second.fi;
    }
    freq_[key] = FreqSlot{};
    const RgbImage& im = f_.at(s);
    auto data = std::make_shared<RgbImage>(im);
    pool_->start([this, data, kind, emph, key]() {
        std::shared_ptr<FreqImages> fi;
        try {
            fi = std::make_shared<FreqImages>(freq_images(data->d, data->H, data->W, kind, emph));
        } catch (...) {
        }
        // the pool is drained in ~Viewer, so `this` is alive here; a queued call to a deleted receiver is dropped
        QMetaObject::invokeMethod(
            this,
            [this, key, fi]() {
                auto it2 = freq_.find(key);
                if (it2 == freq_.end()) return;     // image replaced meanwhile
                it2->second.pending = false;
                it2->second.fi = fi;
                emit computedSignal(QString::fromStdString(key));
            },
            Qt::QueuedConnection);
    });
    return nullptr;
}

void Viewer::on_computed(const QString&) { update(); }

bool Viewer::ready() {
    if (!has_orig_) return true;
    const std::string src = source();
    for (const auto& name : PANES.at(mode_)) {
        auto it = FREQ_KIND.find(name);
        if (it != FREQ_KIND.end()) {
            bool failed = false;
            if (!freq_get(plane_src(src), it->second, &failed) && !failed) return false;
        }
    }
    return true;
}

std::vector<uint8_t> Viewer::pane_pixels(const std::string& name, const std::string& src, int* H, int* W) {
    const int b = brightness_;
    const RgbImage& orig = f_.at("orig");
    auto mm = mnmx_.find(tok_["orig"]);
    if (mm == mnmx_.end()) {
        mnmx_.clear();
        mm = mnmx_.emplace(tok_["orig"], minmax(orig.d)).first;
    }
    const double mn = mm->second.first, mx = mm->second.second;
    *H = orig.H;
    *W = orig.W;
    if (name == "rgb") {
        if (src == "assist") {
            // Filter Tuning Assist: left half original, right half filtered (split() is never "off")
            RgbImage im = f_.at("orig");
            const RgbImage& fl = f_.at("filt");
            const int sw = im.W / 2;
            for (int y = 0; y < im.H; ++y)
                std::memcpy(&im.d[(static_cast<size_t>(y) * im.W + sw) * 3], &fl.d[(static_cast<size_t>(y) * fl.W + sw) * 3],
                            sizeof(float) * 3 * static_cast<size_t>(im.W - sw));
            return render_simple(0, im.d, im.H, im.W, b, mn, mx);
        }
        const RgbImage& im = img(src);
        *H = im.H;
        *W = im.W;
        return render_simple(0, im.d, im.H, im.W, b, mn, mx);
    }
    auto fk = FREQ_KIND.find(name);
    if (fk != FREQ_KIND.end()) {
        auto fi = freq_get(src, fk->second);
        if (!fi) return {};
        int k;
        if (name.back() == '+') {
            k = name == "Y+" ? 0 : name == "Cr+" ? 1 : 2;
        } else {
            const std::string band = name.substr(name.find(' ') + 1);
            k = band == "High" ? 0 : band == "Mid" ? 1 : 2;
        }
        *H = fi->H;
        *W = fi->W;
        return nvqt::render_freq(range_kind(name), *fi, k, b);
    }
    const RgbImage& im = img(src);
    *H = im.H;
    *W = im.W;
    return render_simple(simple_name(name), im.d, im.H, im.W, b, mn, mx);
}

QImage Viewer::pane_image(const std::string& name, const std::string& src) {
    const std::string key = src + "#" + name;
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;
    int H = 0, W = 0;
    const std::vector<uint8_t> px = pane_pixels(name, src, &H, &W);
    if (px.empty()) return QImage();
    QImage qi = to_qimage(px, H, W);
    cache_[key] = qi;
    return qi;
}

QImage Viewer::composite() { return pane_image("rgb", source()); }

std::vector<std::pair<std::string, QRectF>> Viewer::panes() const {
    const QRectF r(rect());
    const auto& comps = PANES.at(mode_);
    if (comps.empty()) return {{"rgb", r}};
    const double g = 3.0, W = r.width(), H = r.height();
    std::vector<std::pair<std::string, QRectF>> out;
    if (split_ == "1+3 Bottom") {
        const double top = (H - g) * 2 / 3, w3 = (W - 2 * g) / 3;
        out.push_back({"rgb", QRectF(0, 0, W, top)});
        for (size_t k = 0; k < comps.size(); ++k)
            out.push_back({comps[k], QRectF(static_cast<double>(k) * (w3 + g), top + g, w3, H - top - g)});
    } else if (split_ == "1+3 Right") {
        const double left = (W - g) * 2 / 3, h3 = (H - 2 * g) / 3;
        out.push_back({"rgb", QRectF(0, 0, left, H)});
        for (size_t k = 0; k < comps.size(); ++k)
            out.push_back({comps[k], QRectF(left + g, static_cast<double>(k) * (h3 + g), W - left - g, h3)});
    } else {
        const double w2 = (W - g) / 2, h2 = (H - g) / 2;
        const QRectF cells[4] = {QRectF(0, 0, w2, h2), QRectF(w2 + g, 0, w2, h2), QRectF(0, h2 + g, w2, h2),
                                 QRectF(w2 + g, h2 + g, w2, h2)};
        out.push_back({"rgb", cells[0]});
        for (size_t k = 0; k < comps.size(); ++k) out.push_back({comps[k], cells[k + 1]});
    }
    return out;
}

double Viewer::scale() const {
    if (!has_orig_) return 1.0;
    if (zoom_ == 0) {
        const QRectF pr = panes()[0].second;
        return std::max(std::min(pr.width() / dW_, pr.height() / dH_), 0.01);
    }
    return zoom_ / 100.0;
}

std::pair<double, double> Viewer::center() const {
    return center_ ? *center_ : std::make_pair(dW_ / 2.0, dH_ / 2.0);
}

std::pair<double, double> Viewer::pane_center(const std::string& name, std::optional<QRectF> pr) const {
    if (name == "rgb") return center();
    double cx, cy;
    if (sel_) {
        auto [x, y, w, h] = coded_to_display(*sel_);
        cx = x + w / 2.0;
        cy = y + h / 2.0;
    } else {
        std::tie(cx, cy) = center();
    }
    if (pr) {
        const double s = scale(), H = dH_, W = dW_;
        const double hw = pr->width() / (2 * s), hh = pr->height() / (2 * s);
        cx = W > 2 * hw ? std::min(std::max(cx, hw), W - hw) : W / 2.0;
        cy = H > 2 * hh ? std::min(std::max(cy, hh), H - hh) : H / 2.0;
    }
    return {cx, cy};
}

QPointF Viewer::to_widget(const QRectF& pane, double x, double y, const std::string& name) const {
    const double s = scale();
    auto [cx, cy] = pane_center(name, pane);
    const QPointF c = pane.center();
    return QPointF(c.x() + (x - cx) * s, c.y() + (y - cy) * s);
}

QPointF Viewer::image_to_widget(double x, double y) const { return to_widget(panes()[0].second, x, y, "rgb"); }

std::tuple<std::string, double, double> Viewer::to_image(QPointF pos, const std::string& only) const {
    for (const auto& [name, pr] : panes())
        if ((only.empty() && pr.contains(pos)) || name == only) {
            const double s = scale();
            auto [cx, cy] = pane_center(name, pr);
            const QPointF c = pr.center();
            return {name, (pos.x() - c.x()) / s + cx, (pos.y() - c.y()) / s + cy};
        }
    return {"", 0, 0};
}

std::tuple<double, double, double, double> Viewer::coded_to_display(const Rect& r) const {
    double pts[2][2];
    const int corners[2][2] = {{r.y0, r.x0}, {r.y0 + r.h, r.x0 + r.w}};
    for (int i = 0; i < 2; ++i) {
        double y = corners[i][0], x = corners[i][1];
        double hh = cH_, ww = cW_;
        for (int k = 0; k < rot_; ++k) {
            const double ny = ww - x, nx = y;
            y = ny;
            x = nx;
            std::swap(hh, ww);
        }
        pts[i][0] = y;
        pts[i][1] = x;
    }
    return {std::min(pts[0][1], pts[1][1]), std::min(pts[0][0], pts[1][0]), std::fabs(pts[1][1] - pts[0][1]),
            std::fabs(pts[1][0] - pts[0][0])};
}

Rect Viewer::display_to_coded(double x0d, double y0d, double wd, double hd) const {
    const int H = cH_, W = cW_;
    const int dh = rot_ % 2 ? W : H, dw = rot_ % 2 ? H : W;
    const int x0 = static_cast<int>(std::max(0.0, std::min(x0d, dw - 1.0)));
    const int y0 = static_cast<int>(std::max(0.0, std::min(y0d, dh - 1.0)));
    const int w = static_cast<int>(std::max(1.0, std::min(wd, static_cast<double>(dw - x0))));
    const int h = static_cast<int>(std::max(1.0, std::min(hd, static_cast<double>(dh - y0))));
    if (rot_ == 0) return {y0, x0, h, w};
    // np.rot90(np.indices((H, W)), rot) restricted to the display rect: min / max of the coded indices
    auto coded = [&](int i, int j, int* y, int* x) {
        if (rot_ == 1) {
            *y = j;
            *x = W - 1 - i;
        } else if (rot_ == 2) {
            *y = H - 1 - i;
            *x = W - 1 - j;
        } else {
            *y = H - 1 - j;
            *x = i;
        }
    };
    int ymin = 1 << 30, ymax = -1, xmin = 1 << 30, xmax = -1;
    for (int i : {y0, y0 + h - 1})
        for (int j : {x0, x0 + w - 1}) {
            int y, x;
            coded(i, j, &y, &x);
            ymin = std::min(ymin, y);
            ymax = std::max(ymax, y);
            xmin = std::min(xmin, x);
            xmax = std::max(xmax, x);
        }
    return {ymin, xmin, ymax - ymin + 1, xmax - xmin + 1};
}

// ---------------------------------------------------------------- painting
void Viewer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(40, 40, 40));
    if (!has_orig_) {
        p.setPen(QColor(170, 170, 170));
        p.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("Open a video (Effect > Open Input Video…)"));
        return;
    }
    const std::string src = source();
    const double s = scale();
    p.setRenderHint(QPainter::SmoothPixmapTransform, s < 1.0);
    QFont f(font());
    f.setBold(true);
    p.setFont(f);
    const auto ps = panes();
    for (size_t k = 0; k < ps.size(); ++k) {
        const auto& [name, pr] = ps[k];
        p.save();
        p.setClipRect(pr);
        p.fillRect(pr, QColor(55, 55, 55));
        const QImage qi = pane_image(name, name == "rgb" ? src : plane_src(src));
        const QPointF tl = to_widget(pr, 0, 0, name);
        const QRectF ir(tl.x(), tl.y(), dW_ * s, dH_ * s);
        if (qi.isNull()) {
            p.setPen(QColor(200, 200, 200));
            p.drawText(pr, Qt::AlignCenter, QString::fromUtf8("computing…"));
        } else {
            p.drawImage(ir, qi);
        }
        overlays(p, pr, name);
        if (src == "pcheck" || src == "nlc") {
            const QString title = QString::fromUtf8(src == "pcheck" ? "⚙ Profile Check" : "⚙ Noise Level Check");
            text(p, pr, title, Qt::AlignTop | Qt::AlignHCenter, PCHECK_TEXT);
            text(p, k == 0 ? ir.intersected(pr) : pr, "Assist Image", Qt::AlignTop);
        } else if (showing_filtered()) {
            text(p, pr, (k == 0 && assist_) ? "Assist Image" : "Filtered", Qt::AlignTop);
        }
        if (name != "rgb") {
            auto it = PANE_LABEL.find(name);
            text(p, pr, QString::fromStdString(it == PANE_LABEL.end() ? name : it->second), Qt::AlignBottom);
        }
        p.restore();
    }
}

void Viewer::text(QPainter& p, const QRectF& pr, const QString& t, int valign, QColor color) {
    const QRectF r = pr.adjusted(6, 4, -8, -4);
    const int align = (valign & Qt::AlignHorizontal_Mask) ? valign : (Qt::AlignRight | valign);
    p.setPen(QColor(0, 0, 0));
    p.drawText(r.translated(1, 1), align, t);
    p.setPen(color);
    p.drawText(r, align, t);
}

QRectF Viewer::wrect(const QRectF& pr, const Rect& rc, const std::string& name) const {
    auto [x, y, w, h] = coded_to_display(rc);
    const QPointF a = to_widget(pr, x, y, name);
    const double s = scale();
    return QRectF(a.x(), a.y(), w * s, h * s);
}

QRectF Viewer::frame(QPainter& p, const QRectF& r, QColor inner, QColor outer, bool dashed) {
    const double t = std::max(1.0, std::min(8.0, py_round(std::min(r.width(), r.height()) / 10.0)));
    p.setBrush(Qt::NoBrush);
    const QRectF out = r.adjusted(-t / 2 - 1, -t / 2 - 1, t / 2 + 1, t / 2 + 1);
    p.setPen(QPen(outer, t + 2, dashed ? Qt::DashLine : Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    p.drawRect(out);
    p.setPen(QPen(inner, t, dashed ? Qt::DashLine : Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    p.drawRect(out);
    return out.adjusted(-t / 2 - 1, -t / 2 - 1, t / 2 + 1, t / 2 + 1);
}

std::optional<QRectF> Viewer::triangle_rect(std::optional<QRectF> pr) const {
    if (!sel_ || sel_style_ != "profile" || !has_orig_) return std::nullopt;
    const QRectF pane = pr ? *pr : panes()[0].second;
    const QRectF r = wrect(pane, *sel_);
    const double t = std::max(1.0, std::min(8.0, py_round(std::min(r.width(), r.height()) / 10.0)));
    return QRectF(r.right() + t / 2 + 2, r.bottom() - 4, 12, 9);
}

void Viewer::overlays(QPainter& p, const QRectF& pr, const std::string& name) {
    const bool primary = name == "rgb";
    if (region_ && primary && showing_filtered() && !sel_) {
        p.setPen(QPen(PREVIEW_IN, 1, Qt::DashLine));
        p.drawRect(wrect(pr, *region_));
    }
    if (!sel_) return;
    const QRectF r = wrect(pr, *sel_, name);
    const int h = sel_->h, w = sel_->w;
    if (!primary) {
        frame(p, r, PANE_SEL_IN, PANE_SEL_OUT);
        return;
    }
    if (sel_style_ != "profile") {
        frame(p, r, PREVIEW_IN, PREVIEW_OUT);
        return;
    }
    QRectF outer;
    if (main_sample_ && *sel_ == *main_sample_) {
        outer = frame(p, r, MAIN_IN, MAIN_OUT);
    } else {
        QColor inner, oc;
        sel_colors(w, h, &inner, &oc);
        outer = frame(p, r, inner, oc, std::min(w, h) <= 15);
    }
    const QFontMetrics fm = p.fontMetrics();
    double y = outer.bottom() + 2;
    if (!sel_label_.isEmpty()) {
        QRectF br(outer.center().x() - 150, outer.top() - fm.height() - 2, 300, fm.height());
        if (br.top() < pr.top()) {
            br.moveTop(y);
            y += fm.height();
        }
        outlined(p, br, sel_label_, QColor(255, 255, 255));
    }
    for (const QString& line : sel_warn_) {
        const QRectF br(outer.center().x() - 200, y, 400, fm.height());
        outlined(p, br, line, WARN);
        y += fm.height();
    }
    if (auto tr = triangle_rect(pr)) {
        p.setPen(QPen(QColor(40, 40, 40), 1));
        p.setBrush(QColor(200, 200, 200));
        p.drawPolygon(QPolygonF({tr->topLeft(), tr->topRight(), QPointF(tr->center().x(), tr->bottom())}));
        p.setBrush(Qt::NoBrush);
    }
}

void Viewer::outlined(QPainter& p, const QRectF& br, const QString& t, QColor color) {
    QPainterPath path;
    const QFontMetrics fm = p.fontMetrics();
    const double x = br.center().x() - fm.horizontalAdvance(t) / 2.0;
    path.addText(QPointF(x, br.top() + fm.ascent()), p.font(), t);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.strokePath(path, QPen(QColor(0, 0, 0), 3));
    p.fillPath(path, color);
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ---------------------------------------------------------------- mouse
void Viewer::set_hold(bool on) {
    if (on != hold_) {
        hold_ = on;
        emit holdChanged(on);
        update();
    }
}

void Viewer::mousePressEvent(QMouseEvent* e) {
    const QPointF pos = e->position();
    if (e->button() == Qt::MiddleButton || (e->button() == Qt::LeftButton && pan_mode)) {
        if (has_orig_) {
            auto [cx, cy] = center();
            pan0_ = Pan{pos, cx, cy, scale()};
        }
        return;
    }
    if (e->button() != Qt::LeftButton || !has_orig_) return;
    if (auto tr = triangle_rect(); tr && tr->adjusted(-3, -3, 3, 3).contains(pos)) {
        emit menuRequested(mapToGlobal(pos.toPoint()));
        return;
    }
    auto [name, x, y] = to_image(pos);
    p0_ = pos;
    g_.reset();
    if (name == "rgb") {
        bool move = false;
        if (sel_ && sel_movable_) {
            auto [sx, sy, sw, sh] = coded_to_display(*sel_);
            move = sx <= x && x <= sx + sw && sy <= y && y <= sy + sh;
            if (move) {
                Gesture g;
                g.kind = "move";
                g.sx = sx;
                g.sy = sy;
                g.sw = sw;
                g.sh = sh;
                g.x = x;
                g.y = y;
                g_ = g;
            }
        }
        if (!move) {
            Gesture g;
            g.kind = "new";
            g.ax = static_cast<int>(std::min(std::max(py_round(x), 0.0), dW_ - 1.0));
            g.ay = static_cast<int>(std::min(std::max(py_round(y), 0.0), dH_ - 1.0));
            g_ = g;
        }
    }
    set_hold(true);
}

std::tuple<int, int, int, int> Viewer::gesture_rect(double x, double y) const {
    const int H = dH_, W = dW_;
    const Gesture& g = *g_;
    if (g.kind == "move") {
        const int nx = static_cast<int>(py_round(std::min(std::max(g.sx + x - g.x, 0.0), W - g.sw)));
        const int ny = static_cast<int>(py_round(std::min(std::max(g.sy + y - g.y, 0.0), H - g.sh)));
        return {nx, ny, static_cast<int>(g.sw), static_cast<int>(g.sh)};
    }
    const int m = sel_max_, ax = g.ax, ay = g.ay;
    const int cx = static_cast<int>(py_round(x)), cy = static_cast<int>(py_round(y));
    int x0, x1, y0, y1;
    if (cx >= ax) {
        x0 = ax;
        x1 = std::min({cx, ax + m, W});
    } else {
        x0 = std::max({cx, ax - m, 0});
        x1 = ax;
    }
    if (cy >= ay) {
        y0 = ay;
        y1 = std::min({cy, ay + m, H});
    } else {
        y0 = std::max({cy, ay - m, 0});
        y1 = ay;
    }
    return {x0, y0, std::max(x1 - x0, 1), std::max(y1 - y0, 1)};
}

void Viewer::mouseMoveEvent(QMouseEvent* e) {
    const QPointF pos = e->position();
    if (pan0_) {
        if (zoom_ == 0) {
            zoom_ = std::nearbyint(pan0_->s * 100.0 * 10.0) / 10.0;   // round(s * 100.0, 1)
            emit viewChanged();
        }
        center_ = std::make_pair(pan0_->cx - (pos.x() - pan0_->p0.x()) / pan0_->s,
                                 pan0_->cy - (pos.y() - pan0_->p0.y()) / pan0_->s);
        update();
        return;
    }
    if (p0_ && g_) {
        const double dx = pos.x() - p0_->x(), dy = pos.y() - p0_->y();
        if (g_->moved || std::max(std::fabs(dx), std::fabs(dy)) > 4) {
            g_->moved = true;
            auto [n, x, y] = to_image(pos, "rgb");
            auto [gx, gy, gw, gh] = gesture_rect(x, y);
            const Rect r = display_to_coded(gx, gy, gw, gh);
            sel_ = r;
            update();
            emit selectionChanging(r.y0, r.x0, r.h, r.w);
        }
        return;
    }
    if (has_orig_) {
        auto [name, x, y] = to_image(pos);
        if (!name.empty() && 0 <= x && x < dW_ && 0 <= y && y < dH_) {
            const RgbImage& im = img(name == "rgb" ? source() : "orig");
            const float* v = &im.d[(static_cast<size_t>(static_cast<int>(y)) * im.W + static_cast<int>(x)) * 3];
            emit cursorText(QString::fromStdString(fmt("R: %.2f", v[0]) + "  " + fmt("G: %.2f", v[1]) + "  " +
                                                   fmt("B: %.2f", v[2])));
        } else {
            emit cursorText("R:  G:  B:");
        }
    }
}

void Viewer::mouseReleaseEvent(QMouseEvent*) {
    if (pan0_) {
        pan0_.reset();
        return;
    }
    if (!p0_) return;
    auto g = g_;
    p0_.reset();
    g_.reset();
    set_hold(false);
    if (g && g->moved && sel_)
        emit areaSelected(sel_->y0, sel_->x0, sel_->h, sel_->w);
    else
        emit clicked();
}

void Viewer::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (zoom_ == 0) emit viewChanged();
}

void Viewer::wheelEvent(QWheelEvent* e) {
    if (has_orig_) zoom_step(e->angleDelta().y() > 0 ? 1 : -1);
}

void Viewer::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape)
        emit clicked();
    else
        QWidget::keyPressEvent(e);
}

// =================================================================== ViewerBar
ViewerBar::ViewerBar(Viewer* viewer, QWidget* parent) : QWidget(parent), v(viewer) {
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(2, 1, 2, 1);
    lay->setSpacing(6);
    zoom = new QToolButton();
    zoom->setPopupMode(QToolButton::InstantPopup);
    zoom->setToolTip("Adjust zoom level of image viewer");
    auto* zm = new QMenu(zoom);
    zm->addAction("Zoom to Fit", [viewer] { viewer->set_zoom(0); });
    zm->addSeparator();
    for (double z : ZOOMS) {
        char b[32];
        std::snprintf(b, sizeof(b), "%g%%", z);
        zm->addAction(b, [viewer, z] { viewer->set_zoom(z); });
    }
    zoom->setMenu(zm);
    zoom->setMinimumWidth(70);
    lay->addWidget(zoom);
    mode = new QToolButton();
    mode->setPopupMode(QToolButton::InstantPopup);
    mode->setToolTip("Adjust image viewer mode");
    mode->setMinimumWidth(80);
    auto* mm = new QMenu(mode);
    auto* g = new QActionGroup(this);
    for (size_t i = 0; i < MODES.size(); ++i) {
        auto sec = MODE_SECTIONS.find(static_cast<int>(i));
        if (sec != MODE_SECTIONS.end()) mm->addSection(QString::fromStdString(sec->second));
        auto* a = new QAction(QString::fromStdString(MODES[i].second), this);
        a->setCheckable(true);
        const std::string sh = MODES[i].first;
        connect(a, &QAction::triggered, this, [viewer, sh] { viewer->set_mode(sh); });
        g->addAction(a);
        mm->addAction(a);
        mode_actions[sh] = a;
    }
    mm->addSection("Viewer Split");
    g = new QActionGroup(this);
    for (const auto& s : SPLITS) {
        auto* a = new QAction(QString::fromStdString(s), this);
        a->setCheckable(true);
        connect(a, &QAction::triggered, this, [viewer, s] { viewer->set_split(s); });
        g->addAction(a);
        mm->addAction(a);
        split_actions[s] = a;
    }
    mode->setMenu(mm);
    lay->addWidget(mode);
    pan = new QToolButton();
    pan->setText(QString::fromUtf8("✋"));
    pan->setCheckable(true);
    pan->setToolTip("Toggle viewer panning mode (or drag with the middle button)");
    connect(pan, &QToolButton::toggled, this, [viewer](bool on) { viewer->pan_mode = on; });
    lay->addWidget(pan);
    lay->addWidget(line());
    bright_reset = new QToolButton();
    bright_reset->setText(QString::fromUtf8("☼"));
    bright_reset->setToolTip("Reset viewer brightness");
    bright = new QSlider(Qt::Horizontal);
    bright->setRange(0, 100);
    bright->setValue(50);
    bright->setFixedWidth(80);
    bright->setToolTip("Adjust viewer brightness (display only; 50 = unchanged)");
    connect(bright, &QSlider::valueChanged, viewer, &Viewer::set_brightness);
    connect(bright_reset, &QToolButton::clicked, this, [this] { bright->setValue(50); });
    lay->addWidget(bright_reset);
    lay->addWidget(bright);
    lay->addWidget(line());
    wh = new QLabel("W:      H:     ");
    wh->setToolTip("Size of the selected area");
    lay->addWidget(wh);
    lay->addWidget(line());
    rgb = new QLabel("R:  G:  B:");
    rgb->setMinimumWidth(200);
    lay->addWidget(rgb);
    lay->addStretch(1);
    lay->addWidget(line());
    size = new QLabel(QString::fromUtf8("Frame: —"));
    lay->addWidget(size);
    info = new QToolButton();
    info->setText(QString::fromUtf8("ⓘ"));
    info->setToolTip("Additional information about image");
    lay->addWidget(info);
    connect(viewer, &Viewer::cursorText, rgb, &QLabel::setText);
    connect(viewer, &Viewer::viewChanged, this, &ViewerBar::sync);
    sync();
}

QFrame* ViewerBar::line() {
    auto* f = new QFrame();
    f->setFrameShape(QFrame::VLine);
    f->setFrameShadow(QFrame::Sunken);
    return f;
}

void ViewerBar::sync() {
    const double pct = v->scale_percent();
    std::string t = fmt("%.1f%%", pct);
    const auto p = t.find(".0%");
    if (p != std::string::npos) t.replace(p, 3, "%");
    zoom->setText(QString::fromStdString(t) + QString::fromUtf8(" ▾"));
    zoom->setToolTip(QString("Adjust zoom level of image viewer") + (v->zoom_fit() ? " (Zoom to Fit)" : ""));
    const std::string& m = v->mode();
    mode->setText(QString::fromStdString(m) + QString::fromUtf8(" ▾"));
    for (auto& [k, a] : mode_actions) a->setChecked(k == m);
    for (auto& [k, a] : split_actions) {
        a->setChecked(k == v->split());
        a->setEnabled(m != "Normal");
    }
}

void ViewerBar::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    sync();
}

void ViewerBar::set_selection(std::optional<Rect> r) {
    wh->setText(!r ? QString("W:      H:     ")
                   : QString::fromStdString("W: " + std::to_string(r->w) + "  H: " + std::to_string(r->h)));
}

void ViewerBar::set_frame_info(int W, int H, int bits) {
    size->setText(QString::fromStdString("Frame: " + std::to_string(W) + "x" + std::to_string(H) + ", " +
                                         std::to_string(bits) + "-bit"));
}

}  // namespace nvqt
