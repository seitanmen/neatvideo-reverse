// viewer - the image viewer of the C++ UI, ported from repro/nv_ui_qt/widgets/viewer.py (Viewer, ViewerBar):
// primary pane + three component panes (YCrCb + Enhanced, RGB, YCrCb, Y / Cr / Cb Frequencies) in 2 x 2 /
// 1+3 Bottom / 1+3 Right, shared zoom, "Filtered" labels, the Prepare page selection (free rectangle, colour by
// min(w, h), Noise Level label, warnings, Build Profile triangle), the Adjust page preview area, Profile Check
// and Noise Level Check sources.  The pane images are display.py's, computed by libnvcore (nvcore.h); the
// Enhanced / Frequencies images in a worker thread, cached per image.
#pragma once

#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QPointF>
#include <QRectF>
#include <QSlider>
#include <QToolButton>
#include <QThreadPool>
#include <QWidget>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "nvcore.h"

namespace nvqt {

extern const std::vector<double> ZOOMS;                                   // 5 .. 800 %
extern const std::vector<std::pair<std::string, std::string>> MODES;      // (short, long)
extern const std::map<int, std::string> MODE_SECTIONS;
extern const std::vector<std::string> SPLITS;

// coded (y0, x0, h, w)
struct Rect {
    int y0 = 0, x0 = 0, h = 0, w = 0;
    bool operator==(const Rect& o) const { return y0 == o.y0 && x0 == o.x0 && h == o.h && w == o.w; }
    bool operator!=(const Rect& o) const { return !(*this == o); }
};

// an RGB float image (H x W x 3) in display orientation
struct RgbImage {
    int H = 0, W = 0;
    std::vector<float> d;
    bool empty() const { return d.empty(); }
};

// np.rot90(img, k) of an H x W x 3 image (k counter-clockwise quarter turns)
RgbImage rot90(const std::vector<float>& rgb, int H, int W, int k);
QImage to_qimage(const std::vector<uint8_t>& rgb, int H, int W);
QImage to_qimage_f(const std::vector<float>& rgb, int H, int W);   // viewer.to_qimage(np.clip(rgb).astype(uint8))

class Viewer : public QWidget {
    Q_OBJECT
public:
    explicit Viewer(QWidget* parent = nullptr);
    ~Viewer() override;

    // state
    const std::string& mode() const { return mode_; }
    void set_mode(const std::string& m);
    const std::string& split() const { return split_; }
    void set_split(const std::string& s);
    // zoom: 0 = "fit", else a percentage
    double zoom() const { return zoom_; }
    bool zoom_fit() const { return zoom_ == 0; }
    void set_zoom(double z);
    void zoom_step(int d);
    double scale_percent() const { return scale() * 100.0; }
    void set_brightness(int b);
    int brightness() const { return brightness_; }
    void set_emphasis(bool on);
    bool emphasis() const { return emphasis_; }
    void set_matrix(const std::string& m) { matrix_ = m; }
    void set_show_filtered(bool on);
    void set_assist(bool on);
    bool showing_filtered() const;
    void set_pcheck(bool on, const std::vector<float>* img = nullptr);
    bool showing_pcheck() const;
    void set_nlc(bool on, const std::vector<float>* img = nullptr, bool keep = true);
    bool showing_nlc() const;
    void set_selection(std::optional<Rect> rect, const std::string& style = "profile", const QString& label = "",
                       const QStringList& warn = {});
    void set_main_sample(std::optional<Rect> rect);
    void set_select_limits(int max_side, bool movable) {
        sel_max_ = max_side;
        sel_movable_ = movable;
    }
    // orig: coded-orientation RGB of the frame (H x W x 3 float); filt: of the frame or of `region`
    void set_images(const std::vector<float>& orig, int H, int W, const std::vector<float>* filt = nullptr,
                    std::optional<Rect> region = std::nullopt, int rot = 0);

    // source of the primary pane: "filt" / "orig" / "assist" / "pcheck" / "nlc"
    std::string source() const;
    bool ready();
    // the pane image (null while its frequency image is computed); name "rgb" = the primary pane
    QImage pane_image(const std::string& name, const std::string& src);
    std::vector<uint8_t> pane_pixels(const std::string& name, const std::string& src, int* H, int* W);
    QImage composite();
    std::vector<std::pair<std::string, QRectF>> panes() const;
    std::optional<QRectF> triangle_rect(std::optional<QRectF> pr = std::nullopt) const;

    bool has_image() const { return has_orig_; }
    // the stored float image of a source ("orig" / "filt" / "pcheck" / "nlc"), display orientation; empty when absent
    const RgbImage& image(const std::string& src) const {
        static const RgbImage none;
        auto it = f_.find(src);
        return it == f_.end() ? none : it->second;
    }
    int disp_H() const { return dH_; }
    int disp_W() const { return dW_; }
    std::optional<Rect> sel() const { return sel_; }
    QString sel_label() const { return sel_label_; }
    QStringList sel_warn() const { return sel_warn_; }
    std::optional<Rect> main_sample() const { return main_sample_; }
    bool hold() const { return hold_; }
    bool pan_mode = false;
    bool assist() const { return assist_; }
    bool pcheck() const { return pcheck_; }
    bool nlc() const { return nlc_; }

    // viewer.sel_colors: (inner, outer) frame colours of a selection by min(w, h)
    static void sel_colors(int w, int h, QColor* inner, QColor* outer);
    // geometry helpers (public for the tests)
    std::tuple<double, double, double, double> coded_to_display(const Rect& r) const;
    Rect display_to_coded(double x0, double y0, double w, double h) const;
    QPointF image_to_widget(double x, double y) const;   // primary pane

signals:
    void areaSelected(int y0, int x0, int h, int w);
    void selectionChanging(int y0, int x0, int h, int w);
    void clicked();
    void holdChanged(bool on);
    void cursorText(QString text);
    void viewChanged();
    void menuRequested(QPoint global);
    void computedSignal(QString key);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private:
    struct FreqSlot {
        bool pending = true;
        std::shared_ptr<FreqImages> fi;
        QString error;
    };
    double scale() const;
    std::pair<double, double> center() const;
    std::pair<double, double> pane_center(const std::string& name, std::optional<QRectF> pr) const;
    QPointF to_widget(const QRectF& pane, double x, double y, const std::string& name = "rgb") const;
    std::tuple<std::string, double, double> to_image(QPointF pos, const std::string& only = "") const;
    QRectF wrect(const QRectF& pr, const Rect& r, const std::string& name = "rgb") const;
    static QRectF frame(QPainter& p, const QRectF& r, QColor inner, QColor outer, bool dashed = false);
    void overlays(QPainter& p, const QRectF& pr, const std::string& name);
    static void text(QPainter& p, const QRectF& pr, const QString& t, int valign, QColor color = QColor(255, 255, 255));
    static void outlined(QPainter& p, const QRectF& br, const QString& t, QColor color);
    void set_hold(bool on);
    std::string plane_src(const std::string& src) const;
    const RgbImage& img(const std::string& src) const;
    std::shared_ptr<FreqImages> freq_get(const std::string& src, int kind, bool* failed = nullptr);
    void drop_freq();
    std::tuple<int, int, int, int> gesture_rect(double x, double y) const;
    void on_computed(const QString& key);

    std::map<std::string, RgbImage> f_;            // "orig" / "filt" / "pcheck" / "nlc"
    std::map<std::string, long> tok_{{"orig", 0}, {"filt", 0}, {"pcheck", 0}, {"nlc", 0}};
    long ntok_ = 0;
    bool has_orig_ = false, has_filt_ = false;
    int dH_ = 0, dW_ = 0;                           // display-oriented size of the frame
    std::optional<Rect> region_;
    int rot_ = 0;
    int cH_ = 0, cW_ = 0;                           // coded shape
    std::string matrix_ = "bt709";
    std::string mode_ = "YCrCb +", split_ = "1+3 Bottom";
    double zoom_ = 0;                               // 0 = fit
    std::optional<std::pair<double, double>> center_;
    int brightness_ = 50;
    bool emphasis_ = true, show_filtered_ = false, assist_ = false, pcheck_ = false, nlc_ = false, hold_ = false;
    int sel_max_ = 512;
    bool sel_movable_ = true;
    std::optional<Rect> sel_;
    std::string sel_style_ = "profile";
    QString sel_label_;
    QStringList sel_warn_;
    std::optional<Rect> main_sample_;
    std::map<std::string, QImage> cache_;
    std::map<std::string, FreqSlot> freq_;          // "<token>|<kind>|<emph>"
    std::map<long, std::pair<double, double>> mnmx_;
    // gestures
    std::optional<QPointF> p0_;
    struct Gesture {
        std::string kind;   // "new" / "move"
        double sx = 0, sy = 0, sw = 0, sh = 0, x = 0, y = 0;
        int ax = 0, ay = 0;
        bool moved = false;
    };
    std::optional<Gesture> g_;
    struct Pan {
        QPointF p0;
        double cx, cy, s;
    };
    std::optional<Pan> pan0_;
    QThreadPool* pool_ = nullptr;
};

class ViewerBar : public QWidget {
    Q_OBJECT
public:
    explicit ViewerBar(Viewer* viewer, QWidget* parent = nullptr);
    void sync();
    void set_selection(std::optional<Rect> rect);
    void set_frame_info(int W, int H, int bits = 8, bool is_float = false);   // "Frame: WxH, N-bit[ float]"

    Viewer* v;
    QToolButton *zoom, *mode, *pan, *bright_reset, *info;
    QSlider* bright;
    QLabel *wh, *rgb, *size;
    std::map<std::string, QAction*> mode_actions, split_actions;

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    static QFrame* line();
};

}  // namespace nvqt
