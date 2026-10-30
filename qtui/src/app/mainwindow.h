// mainwindow - the main window of the C++ UI, ported from repro/nv_ui_qt/main_window.py (genuine layout: menu
// bar with the Beginner / Advanced switch, page tabs Prepare Noise Profile | Adjust and Preview, left column =
// page toolbar -> viewer -> viewer bar, right = side panel, bottom row = scan type / gamma / gain (n/a),
// thumbnails, Cancel, Apply; status bar).  N3a: the skeleton and the Prepare Noise Profile page (open video,
// selection, Auto / Build Profile, Auto Fine-Tune, the Device Noise Profile panel, Profile Check) and the filtered
// preview of the Adjust page with the current parameters; the Filter Settings panel, export, Preferences, Profile
// Viewer, Noise Level Check, Batch Profiler and the OFX mode are N3b / N3c (their actions are shown disabled).
#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>

#include <atomic>
#include <functional>
#include <list>
#include <set>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "engine.h"
#include "export.h"
#include "filter_panel.h"
#include "nvcore.h"
#include "state.h"
#include "video.h"
#include "viewer.h"
#include "worker.h"

namespace nvqt {

class ProfilePanel;
class Timeline;
class IconBar;

// QSettings (INI) of the UI: NV_UI_QT_SETTINGS=<file> selects another file (tests), else the Python UI's file
// (NeatRepro / nv_ui_qt), so the default profile / preset and the preferences are shared with it.
std::unique_ptr<QSettings> settings();

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& video = QString(), QWidget* parent = nullptr);
    ~MainWindow() override;

    AppState* state;
    TaskRunner* runner;
    std::vector<Frame> frames;
    std::optional<VideoInfo> info;
    std::unique_ptr<PreviewEngine> engine;
    QString src;
    std::string matrix = "bt709";
    bool full_range = false;
    std::optional<Rect> sel_area;        // Prepare page selection (y0, x0, h, w)
    std::optional<Rect> preview_sel;     // Adjust page preview area
    std::atomic<long> prof_req{0};
    long gen = 0, prof_token = 0;
    std::map<std::string, QVariant> prefs;
    std::map<std::string, QAction*> actions;
    bool advanced = true;
    bool pcheck = false;
    bool nlc = false;                     // Noise Level Check (temporal filter mask) running
    ExportJob* export_ = nullptr;         // the running export (nullptr = none)
    QString export_dst;
    bool export_cancelled = false, export_ok = false;
    std::vector<QString> export_files;
    double speed_result = 0.0;            // Check Speed: frames / s
    std::string depth_used = "8";         // processing depth of the frames (Preferences > Bit depth when loaded)
    std::optional<PixFmt> fmt;            // processing format of the loaded frames (none: no video / OFX host)
    int ofx_depth = 0;                    // OFX host mode: --depth 8 / 16 / 32 (0 = standalone)
    std::optional<int> quality_pct, completeness_pct;
    std::string prof_clean;              // the signature of the saved / loaded profile ("" = none)

    // widgets
    QTabBar* tabs;
    QStackedWidget *tb_stack, *stack;
    Viewer* viewer;
    ViewerBar* viewer_bar;
    ProfilePanel* profile_panel;
    FilterPanel* filter_panel;
    Timeline* timeline;
    QPushButton *b_cancel, *b_apply, *cancel_btn;
    QLabel *msg, *video_lbl, *depth_lbl, *prof_lbl;
    QProgressBar* prog;
    QToolButton *auto_btn, *load_btn, *generic_btn, *check_btn, *variants_btn;
    QMenu* auto_menu;
    QToolButton *area_btn, *play_btn, *preview_btn, *fsc_btn;
    QAction *fsc_nlc, *fsc_assist;

    // actions (main_window.py names)
    int page() const;
    void say(const QString& text);
    void set_advanced(bool adv);
    void open_video(const QString& path = QString());
    void auto_profile(bool noisy = false);
    QWidget* build_profile(bool noisy = false);   // the message box shown, if any
    void auto_fine_tune();
    void auto_complete();
    void reset_fine_tune();
    void reset_profile();
    void generic_profile();
    void edit_device(const QString& name);
    void on_area(int y, int x, int h, int w);
    void set_preview_area(int y, int x, int h, int w);
    void set_profile_check(bool on);
    void set_assist(bool on);
    void set_noise_level_check(bool on);
    std::string nlc_key() const;
    void request_nlc();
    // for the tests / dumps: the shown mask is the current key's and no recomputation is pending; a preview is queued
    bool nlc_ready() const { return !nlc_timer->isActive() && nlc_shown && *nlc_shown == nlc_key(); }
    bool preview_pending() const { return timer->isActive(); }
    // no preview / selection / Profile Check / Noise Level Check recomputation pending (tests, drivers)
    bool timers_idle() const {
        return !timer->isActive() && !sel_timer->isActive() && !pc_timer->isActive() && !nlc_timer->isActive();
    }
    const std::optional<std::string>& nlc_shown_key() const { return nlc_shown; }
    // for the tests: the Assist Image of the current frame and profile is in the Profile Check cache
    // (main_window._pc_key() in _pc_cache)
    bool pc_cached() const;
    void check_speed();
    void performance_tab();
    void reset_spatial();
    // export (process_video / cancel_export of main_window.py); dst empty = ask
    void process_video(const QString& dst = QString(), std::optional<int> start = std::nullopt,
                       std::optional<int> frames = std::nullopt);
    void cancel_export();
    std::pair<QString, QString> export_filter() const;   // (save-dialog filter, default extension)
    void save_profile(const QString& path = QString());
    void load_profile(const QString& path = QString());
    void copy_profile();
    void paste_profile();
    void assign_default_profile();
    void reset_default_profile();
    void change_default_profile(const QString& path = QString());
    void save_preset(const QString& path = QString());
    void load_preset(const QString& path = QString());
    void reset_filter();
    QWidget* preset_description();
    void copy_both();
    void paste_both();
    void copy_filter();
    void paste_filter();
    void assign_default_preset();
    void reset_default_preset();
    void change_default_preset(const QString& path = QString());
    void save_frame(const QString& path = QString());
    QWidget* guide(const std::string& which);
    // N3c: Tools > Preferences…, Profile > Profile Viewer…, Tools > Batch Profiler… (files + outdir: run at once)
    QDialog* preferences();
    QDialog* profile_viewer();
    QDialog* batch_profiler(const QStringList* files = nullptr, const QString& outdir = QString(),
                            const QString& mode = QString());
    std::optional<std::vector<QString>> batch_result;   // the profiles the last batch wrote
    std::string depth() const;                          // Preferences > Bit depth as a --depth value
    QWidget* about();
    QWidget* image_info();
    void cycle_split(int d);
    void invalidate();
    void schedule();
    void display();
    bool previewing() const;
    std::optional<Crop> crop() const;
    void on_page(int i);
    void sync_selection();
    std::string profile_sig() const;
    // the Python UI's displayed values (for the comparisons with it, qtui/tools/compare_ui.py)
    QString dump_state();
    // OFX host mode (ofx.cpp, main.py _ofx_main): the host's frames instead of a video, and the standalone-only
    // controls off (main_window._set_ofx_mode)
    void set_ofx_frames(std::vector<Frame>&& fr, int W, int H, double fps, int depth);
    void set_ofx_mode(bool ofx);
    std::function<void(QCloseEvent*)> close_hook;         // before the close clean-up (OFX: write the frame)
    std::function<void(QCloseEvent*)> after_close_hook;   // after it (OFX: quit the application)

    // numbers of the selection (display.noise_level / not_uniform), cached
    std::optional<double> nl(int f, std::optional<Rect> rect);
    const std::vector<float>& frame_rgb(int t);
    QImage thumb(int i);

signals:
    void profileBuilt();
    void aftFinished();
    void videoLoaded();

protected:
    void closeEvent(QCloseEvent* e) override;

private:
    QWidget* prepare_toolbar();
    QWidget* adjust_toolbar();
    void build_menus();
    void update_profile_button();
    void auto_clicked();
    void update_radius();
    void sync_view_actions();
    void load_prefs();
    void failed(const QString& tag, const QString& msg);
    void progress(double pct, const QString& text);
    void request_preview();
    void on_frame(int t);
    void on_params(const QStringList& keys);
    void on_profile();
    void update_quality();
    void viewer_area(int y, int x, int h, int w);
    void viewer_changing(int y, int x, int h, int w);
    void viewer_click();
    QMenu* sel_menu(QPoint pos);
    void area_toggled(bool on);
    void play(bool on);
    void stop_initial();
    void request_pcheck();
    void profile_job(bool noisy, std::optional<Rect> area, const QString& text);
    void busy(const std::string& slot, bool on);
    QWidget* info_box(const QString& title, const QString& text);
    QString ask_open(const QString& title, const QString& filt);
    QString ask_save(const QString& title, const QString& filt, const QString& name = QString());
    PyObj default_preset();
    void preset_named(const QString& name, const PyObj& params);
    std::string profile_mode() const;
    void update_performance();
    void sync_nlc_widgets();
    void link(const QString& href);
    void export_done(bool ok, const QString& text);
    static void remove_export_files(const std::vector<QString>& files);
    void run_batch(const QStringList& files, const QString& outdir, const QString& mode);
    void install_frames();
    QString depth_text() const;

    int page_ = 0;
    std::map<int, std::string> view_modes{{0, "YCrCb +"}, {1, "Normal"}};
    std::map<std::string, std::vector<float>> filt_cache;   // "t|gen|crop" -> RGB of the frame / crop
    std::list<std::string> filt_order;
    std::map<int, QImage> thumb_cache;
    std::list<int> thumb_order;
    std::map<std::string, double> nl_cache;
    std::list<std::string> nl_order;
    QToolButton* mode_button_ = nullptr;
    std::map<std::string, std::pair<std::vector<std::string>, std::vector<std::string>>> nu_cache;
    std::optional<std::pair<int, std::vector<float>>> cur_rgb;
    std::map<std::string, std::vector<float>> pc_cache;
    std::list<std::string> pc_order;
    std::set<std::string> busy_slots;
    QString preset_name = "default";
    PyObj preset_clean;
    int play_start = 0;
    long quality_req = 0;
    std::optional<std::string> quality_key;
    QTimer *timer, *sel_timer, *pc_timer, *nlc_timer;
    long engine_id = 0;
    std::map<std::string, std::pair<std::vector<float>, double>> nlc_cache;   // nlc_key -> (mask image RGB, mean)
    std::list<std::string> nlc_order;
    std::optional<std::string> nlc_shown;   // key of the mask image in the viewer
    std::optional<std::string> nlc_mode;    // viewer mode before Noise Level Check
};

}  // namespace nvqt
