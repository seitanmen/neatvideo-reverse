// qtui_selftest - Qt Test self-test of the C++ UI (N3a), off-screen (no window is shown).
//
//   QT_QPA_PLATFORM=offscreen qtui/build/qtui_selftest [-v2]        (video: $NV_TEST_VIDEO or <repo>/test.mp4)
//
// The items follow repro/nv_ui_qt/selftest.py (the Python names are in the comments; N3a, N3b and N3c items); the
// bit-exact agreement with the Python UI itself is checked by qtui/tools/compare_ui.py / compare_n3c.py /
// compare_export.py / compare_ofx.py.  The OFX items run qtui_ofx_drv (built next to this binary).
// Writes only into a temporary directory (settings, profile / preset files).
#include <QApplication>
#include <QWheelEvent>
#include <QSlider>
#include <QScrollArea>
#include <QTextEdit>
#include <QSpinBox>
#include <QScrollBar>
#include <QProcess>
#include <QListWidget>
#include <QLabel>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>

#include <QDir>
#include <QMenu>
#include <QRegularExpression>

#include <cmath>
#include <random>
#include <thread>
#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>

#include "depth.h"
#include "export.h"
#include "filter_panel.h"
#include "noise_graph.h"
#include "mainwindow.h"
#include "profile_panel.h"
#include "pyobj.h"
#include "rows.h"
#include "state.h"
#include "nvcore_c_ui.h"
#include "nvcore_c_rgb.h"
#include "nvcore.h"
#include "timeline.h"
#include "video.h"

using namespace nvqt;

namespace {

bool wait_until(const std::function<bool()>& cond, double timeout_s = 300) {
    QElapsedTimer t;
    t.start();
    while (!cond()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 20);
        if (t.elapsed() > timeout_s * 1000) return false;
    }
    QCoreApplication::processEvents();
    return true;
}

QString test_video() {
    const QByteArray e = qgetenv("NV_TEST_VIDEO");
    return e.isEmpty() ? QString(NV_SOURCE_ROOT) + "/test.mp4" : QString::fromLocal8Bit(e);
}

}  // namespace

class SelfTest : public QObject {
    Q_OBJECT

    QTemporaryDir tmp;
    MainWindow* win = nullptr;
    struct OfxRun {
        int code = -1;
        QByteArray out, err;
    };
    PyObj g1_aft_;          // the AFT profile of the G1 items
    QString v10;            // the 10-bit source of the Bit depth items and its raw samples
    QByteArray a10;

    bool idle() const { return win->runner->idle(); }
    bool aft_done() const {
        return win->state->profile.has_profile() && win->state->profile.base.get("aft_applied").truthy() && idle();
    }
    // a press / drag / release on the primary pane, in coded image coordinates
    void drag(QPointF a, QPointF b) {
        const QPoint pa = win->viewer->image_to_widget(a.x(), a.y()).toPoint();
        const QPoint pb = win->viewer->image_to_widget(b.x(), b.y()).toPoint();
        QTest::mousePress(win->viewer, Qt::LeftButton, Qt::NoModifier, pa);
        QTest::mouseMove(win->viewer, (pa + pb) / 2);
        QTest::mouseMove(win->viewer, pb);
        QTest::mouseRelease(win->viewer, Qt::LeftButton, Qt::NoModifier, pb);
        QCoreApplication::processEvents();
    }

private slots:
    void initTestCase() {
        QVERIFY(tmp.isValid());
        const QString ini = tmp.filePath("settings.ini");
        qputenv("NV_UI_QT_SETTINGS", ini.toLocal8Bit());
        QSettings s(ini, QSettings::IniFormat);
        s.setValue("prefs/mem_gb", 0.05);           // test.mp4: the first 155 frames
        s.sync();
    }

    // ---------------- pure functions
    void pyFloatRepr() {   // the profile files are json.dumps text: repr(float)
        QCOMPARE(py_float_repr(0.1), std::string("0.1"));
        QCOMPARE(py_float_repr(2.0), std::string("2.0"));
        QCOMPARE(py_float_repr(-0.0), std::string("-0.0"));
        QCOMPARE(py_float_repr(1e-05), std::string("1e-05"));
        QCOMPARE(py_float_repr(0.0001), std::string("0.0001"));
        QCOMPARE(py_float_repr(1e16), std::string("1e+16"));
        QCOMPARE(py_float_repr(1234567890123456.0), std::string("1234567890123456.0"));
        QCOMPARE(py_float_repr(1.2345678901234568e+17), std::string("1.2345678901234568e+17"));
        QCOMPARE(py_float_repr(1.0 / 3.0), std::string("0.3333333333333333"));
        QCOMPARE(py_float_repr(std::nan("")), std::string("NaN"));
        QCOMPARE(py_float_repr(-INFINITY), std::string("-Infinity"));
        QCOMPARE(py_fixed(3.25, 1), std::string("3.2"));   // correctly rounded as Python (Qt's asprintf gives 3.3)
    }

    void pyJsonRoundTrip() {   // json.dump(obj, fh, indent=1): key order, ints vs floats, escapes
        const std::string text = "{\"a\": [1, 2.5, {\"b\": null}], \"c\": true, \"d\": \"x\\u00e9\", \"e\": 1.0}";
        const PyObj o = PyObj::parse(text);
        QCOMPARE(o.keys(), (std::vector<std::string>{"a", "c", "d", "e"}));
        QCOMPARE(o.get("a")[0].type(), PyObj::INT);
        QCOMPARE(o.get("e").type(), PyObj::FLOAT);
        QCOMPARE(o.dump(), text);
        const std::string ind = "{\n \"a\": [\n  1,\n  2.5,\n  {\n   \"b\": null\n  }\n ],\n \"c\": true,\n \"d\": "
                                "\"x\\u00e9\",\n \"e\": 1.0\n}";
        QCOMPARE(o.dump(1), ind);
        PyObj d = o;
        d.set("a", PyObj::integer(7));              // an existing key keeps its place (Python dict)
        QCOMPARE(d.keys().front(), std::string("a"));
        QVERIFY(PyObj::parse("{\"x\": 1}") == PyObj::parse("{\"x\": 1.0}"));   // Python ==
    }

    void defaultsMapToV5() {   // "UI defaults map to nv.V5_DEFAULTS"
        const PyObj P = backend_params(default_params());
        QCOMPARE(P.get("radius").as_int(), 2);
        QCOMPARE(P.get("temporal").as_double(), 1.0);
        QCOMPARE(P.get("threshold").as_double(), 4.0);
        QCOMPARE(P.get("spatial").as_double(), 2.0);
        QCOMPARE(P.get("luma").as_double(), 0.7);
        QCOMPARE(P.get("chroma").as_double(), 1.0);
        QCOMPARE(P.get("noise_level").as_double(), 1.0);
        QCOMPARE(P.get("mix").as_double(), 0.0);
        QCOMPARE(P.get("similarity").as_str(), std::string("v5"));
        QCOMPARE(P.get("bs").as_int(), 16);
        QCOMPARE(P.get("search").as_int(), 16);
        QCOMPARE(P.get("pyramid_scale").as_double(), 1.5);
        for (const auto& b : V5_BANDS) {
            QCOMPARE(P.get("band_amounts").get(b).as_double(), b == "ultra-low" ? 0.0 : 1.0);
            QCOMPARE(P.get("band_noise").get(b).as_double(), 1.0);
        }
        QCOMPARE(P.get("band_amounts").keys(), V5_BANDS);
    }

    void coerceRules() {
        QCOMPARE(coerce("t_radius", PyObj::real(2.5)).as_int(), 2);    // round half to even, as Python round
        QCOMPARE(coerce("t_radius", PyObj::real(9)).as_int(), 5);
        QCOMPARE(coerce("amt_Y", PyObj::real(-4)).as_double(), 0.0);
        QCOMPARE(coerce("adv_similarity", PyObj::str("bogus")).as_str(), std::string("v5"));
        QCOMPARE(coerce("t_enable", PyObj::integer(0)).truthy(), false);
    }

    void autoCompleteBins() {   // "manual fine-tune + auto complete"
        Factors f = unit_factors();
        std::array<std::set<int>, 3> pins;
        f[0][2] = 1.5;
        f[0][6] = 0.5;
        pins[0] = {2, 6};
        const Factors g = auto_complete(f, pins);
        QCOMPARE(g[0][2], 1.5);
        QCOMPARE(g[0][6], 0.5);
        QCOMPARE(g[0][0], 1.5);            // constant beyond the outermost edited bin
        QCOMPARE(g[0][8], 0.5);
        QCOMPARE(g[0][4], 1.0);            // linear between
        QCOMPARE(g[1], unit_factors()[1]);
    }

    void selectionColours() {   // "frame colours by min(w, h) (<= 15 red, 16..127 yellow, >= 128 green)"
        QColor in, out;
        Viewer::sel_colors(15, 300, &in, &out);
        QCOMPARE(in, QColor(255, 15, 15));
        Viewer::sel_colors(16, 127, &in, &out);
        QCOMPARE(in, QColor(255, 208, 15));
        Viewer::sel_colors(128, 512, &in, &out);
        QCOMPARE(in, QColor(15, 192, 15));
    }

    // ---------------- the window
    void loadAndAutoProfile() {   // "load + auto profile" (+ the automatic Auto Fine-Tune)
        win = new MainWindow(test_video());
        win->show();
        QVERIFY(wait_until([&] { return aft_done(); }));
        QCOMPARE(static_cast<int>(win->frames.size()), 155);
        QVERIFY(win->state->profile.base.get("area").size() >= 3);
        QCOMPARE(win->timeline->frame(), std::min(155 / 3, 155 - 3));
        QVERIFY(win->video_lbl->text().contains("640x360"));
        QVERIFY(win->depth_lbl->text().startsWith("source yuv420p (8-bit 4:2:0)"));
        QVERIFY(win->prof_lbl->text().startsWith(QString::fromUtf8("Unspecified device · auto · σ Y=")));
    }

    void menusInGenuineOrder() {   // "menus Effect View Profile Filter Variants Tools Help"
        QStringList titles;
        for (QAction* a : win->menuBar()->actions()) titles << a->text();
        QCOMPARE(titles, (QStringList{"E&ffect", "&View", "&Profile", "F&ilter", "V&ariants", "T&ools", "&Help"}));
        QVERIFY(win->actions.count("p_check") && win->actions.count("undo") && win->actions.count("redo"));
        QCOMPARE(win->actions["advanced"]->isChecked(), true);
    }

    void toolbarsInGenuineOrder() {   // "toolbars in the genuine order"
        QStringList t0, t1;
        for (QToolButton* b : win->tb_stack->widget(0)->findChildren<QToolButton*>(Qt::FindDirectChildrenOnly)) t0 << b->text();
        for (QToolButton* b : win->tb_stack->widget(1)->findChildren<QToolButton*>(Qt::FindDirectChildrenOnly)) t1 << b->text();
        // main_window._prepare_toolbar / _adjust_toolbar: Auto, Generic, Load | Profile Check, Variants;
        // area, Preview, Play All | Filter Settings Check, Variants
        QCOMPARE(t0, (QStringList{"Auto Profile", "Generic Profile", "Load Profile", "Profile Check", "Variants"}));
        QCOMPARE(t1, (QStringList{QString::fromUtf8("⛶"), "Preview", "Play All", "Filter Settings Check", "Variants"}));
    }

    void qualityPanel() {   // "Quality: panel shows a percent for the AFT profile" / "label matches recomputation"
        QVERIFY(wait_until([&] { return idle() && win->quality_pct.has_value(); }));
        const auto qc = profile_numbers(win->state->profile, win->frames, win->full_range, win->matrix);
        QVERIFY(qc.has_value());
        QCOMPARE(win->profile_panel->quality->text(), QString("%1%").arg(qc->first));
        QCOMPARE(win->profile_panel->completeness->text(), QString("%1%").arg(qc->second));
        QVERIFY(qc->second > 1);                    // AFT marks the unmeasured bins
    }

    void resetFineTuneCompleteness() {   // "Completeness: after Reset the LZMA flag is cleared -> 0%"
        win->reset_fine_tune();
        QVERIFY(wait_until([&] { return idle(); }));
        QCOMPARE(win->profile_panel->completeness->text(), QString("0%"));
        win->state->undo->undo();
        QVERIFY(wait_until([&] { return idle(); }));
        QVERIFY(win->profile_panel->completeness->text() != "0%");
    }

    void viewerModePerPage() {   // "viewer mode kept per page (defaults Prepare YCrCb+, Adjust Normal)"
        QCOMPARE(win->viewer->mode(), std::string("YCrCb +"));
        win->viewer->set_mode("RGB");
        win->tabs->setCurrentIndex(1);
        QCOMPARE(win->viewer->mode(), std::string("Normal"));
        QCOMPARE(win->actions["split_2 x 2"]->isEnabled(), false);   // no split in Normal
        win->tabs->setCurrentIndex(0);
        QCOMPARE(win->viewer->mode(), std::string("RGB"));
        win->viewer->set_mode("YCrCb +");
        QCOMPARE(win->actions["split_2 x 2"]->isEnabled(), true);
    }

    void splitsArrangePanes() {   // "(f) YCrCb+ draws 4 panes; split changes the arrangement"
        QCOMPARE(win->viewer->panes().size(), size_t(4));
        const QRectF bottom = win->viewer->panes()[1].second;
        win->viewer->set_split("1+3 Right");
        QVERIFY(win->viewer->panes()[1].second.left() > win->viewer->width() / 2);
        win->viewer->set_split("2 x 2");
        QVERIFY(win->viewer->panes()[1].second != bottom);
        win->viewer->set_split("1+3 Bottom");
        QVERIFY(wait_until([&] { return win->viewer->ready(); }, 120));
        QVERIFY(!win->viewer->pane_image("Y+", "orig").isNull());
    }

    void selectionByMouse() {   // "free rectangle by mouse (corner = press point), Noise Level label"
        drag(QPointF(300, 40), QPointF(140, 200));  // dragging up-left: the press point is the lower right corner
        QVERIFY(win->sel_area.has_value());
        const Rect r = *win->sel_area;
        QCOMPARE(r.x0 + r.w, 300);
        QCOMPARE(r.y0, 40);
        QVERIFY(std::abs(r.w - 160) <= 1 && std::abs(r.h - 160) <= 1);
        QVERIFY(win->viewer->sel_label().startsWith("Noise Level: "));
        QVERIFY(win->viewer->sel_label().contains("%)"));             // (+n%) against the Main Sample
        QCOMPARE(win->auto_btn->text(), QString("Build Profile"));
        QCOMPARE(win->viewer_bar->wh->text(), QString("W: %1  H: %2").arg(r.w).arg(r.h));
    }

    void selectionMoveAndClear() {   // "move inside, click clears"
        const Rect r = *win->sel_area;
        drag(QPointF(r.x0 + r.w / 2.0, r.y0 + r.h / 2.0), QPointF(r.x0 + r.w / 2.0 + 30, r.y0 + r.h / 2.0 + 20));
        QVERIFY(win->sel_area.has_value());
        QCOMPARE(win->sel_area->w, r.w);
        QCOMPARE(win->sel_area->h, r.h);
        QVERIFY(std::abs(win->sel_area->x0 - (r.x0 + 30)) <= 1);
        const QPoint c = win->viewer->image_to_widget(500, 300).toPoint();
        QTest::mouseClick(win->viewer, Qt::LeftButton, Qt::NoModifier, c);
        QVERIFY(!win->sel_area.has_value());
        QCOMPARE(win->auto_btn->text(), QString("Auto Profile"));
    }

    void buildProfileSizeRules() {   // "Build Profile disabled / error under 32 px, ... (32..127 confirmation)"
        const std::string sig0 = win->profile_sig();
        win->on_area(10, 10, 20, 60);
        QCOMPARE(win->auto_btn->isEnabled(), false);
        QWidget* box = win->build_profile(false);
        QVERIFY(box && qobject_cast<QMessageBox*>(box));
        QVERIFY(qobject_cast<QMessageBox*>(box)->text().startsWith("Too small area"));
        box->close();
        win->on_area(10, 10, 64, 64);
        QVERIFY(win->auto_btn->isEnabled());
        box = win->build_profile(false);
        auto* mb = qobject_cast<QMessageBox*>(box);
        QVERIFY(mb);
        bool has_use = false;
        for (QAbstractButton* b : mb->buttons()) has_use = has_use || b->text() == "Use the small area";
        QVERIFY(has_use);
        box->close();
        QVERIFY(wait_until([&] { return idle(); }));
        QCOMPARE(win->profile_sig(), sig0);                                  // nothing built (L4: not by the area's size)
    }

    void notUniformAndTriangle() {   // "Not Uniform label, triangle menu"
        win->on_area(0, 0, 360, 512);
        QVERIFY(!win->viewer->sel_warn().isEmpty());
        QVERIFY(win->viewer->sel_warn()[0].startsWith("Not Uniform: ") || win->viewer->sel_warn()[0].startsWith("Clipping: "));
        const auto tr = win->viewer->triangle_rect();
        QVERIFY(tr.has_value());
        QSignalSpy spy(win->viewer, &Viewer::menuRequested);
        QTest::mouseClick(win->viewer, Qt::LeftButton, Qt::NoModifier, tr->center().toPoint());
        QCOMPARE(spy.count(), 1);
        for (QWidget* w : QApplication::topLevelWidgets())
            if (auto* m = qobject_cast<QMenu*>(w)) m->close();
    }

    void buildProfileFromSelection() {   // "Build Profile (Current Frame)" / "rectangular Build Profile: area kept"
        win->on_area(40, 60, 128, 160);
        win->build_profile(false);
        QVERIFY(wait_until([&] {
            const PyObj& b = win->state->profile.base;
            return aft_done() && b.get("area").size() == 4 && b.get("area")[1].as_int() == 60;
        }));
        QCOMPARE(win->state->profile.base.get("area").dump(), std::string("[40, 60, 128, 160]"));
        QVERIFY(win->state->profile.base.get("flat_status").is_none());     // a manual area
        QVERIFY(win->viewer->sel_label().endsWith("(+0%)"));               // the selection is the Main Sample
    }

    void undoRedoProfile() {   // "undo AFT" / redo
        const std::string after = win->profile_sig();
        win->state->undo->undo();                  // the AFT
        QVERIFY(!win->state->profile.base.get("aft_applied").truthy());
        win->state->undo->undo();                  // the build
        QVERIFY(win->state->profile.base.get("area").size() == 3);
        win->state->undo->redo();
        win->state->undo->redo();
        QCOMPARE(win->profile_sig(), after);
    }

    void aftTwiceEqualsOnce() {   // "Auto Fine-Tune twice == once (no squared factors)"
        const PyObj before = win->state->profile.base.get("curve");
        win->auto_fine_tune();
        QVERIFY(wait_until([&] { return idle(); }));
        QVERIFY(win->state->profile.base.get("curve") == before);
        QVERIFY(win->profile_panel->b_aft->isEnabled());                // "AFT buttons re-enabled"
    }

    void profileNoiseLevel() {   // "(d) profile Noise Level scales the curve, undo, saved in JSON"
        const PyObj base_curve = win->state->profile.base.get("curve").get("Y");
        win->state->new_edit();
        win->state->set_tune("all", 1.5);
        const PyObj* eff = win->state->effective();
        QVERIFY(eff && !(eff->get("curve").get("Y") == base_curve));
        QCOMPARE(win->profile_panel->tune.at("all")->val->text(), QString("+50%"));
        QVERIFY(win->state->profile.to_json().get("tune").has("all"));
        win->state->undo->undo();
        QCOMPARE(win->state->profile.tune_value("all"), 1.0);
        QCOMPARE(win->profile_panel->tune.at("all")->val->text(), QString("+0%"));
    }

    void manualFineTune() {   // "fine-tune changes the effective curve"
        std::array<double, 9> f;
        f.fill(1.0);
        f[4] = 2.0;
        win->state->new_edit();
        win->state->set_factors(0, f, 4);
        QVERIFY(win->state->profile.pinned[0].count(4));
        const PyObj* eff = win->state->effective();
        QVERIFY(eff && eff->has("ui_factors"));
        QVERIFY(!(eff->get("curve").get("Y") == win->state->profile.base.get("curve").get("Y")));
        win->state->undo->undo();
    }

    void profileSaveLoad() {   // "profile save/load" / "malformed profile / preset files rejected"
        const QString p = tmp.filePath("p.nvprof.json");
        win->save_profile(p);
        const std::string sig = win->profile_sig();
        win->reset_profile();
        QVERIFY(!win->state->profile.has_profile());
        win->load_profile(p);
        QCOMPARE(win->profile_sig(), sig);
        QVERIFY(!win->profile_panel->name->currentText().endsWith("*"));   // saved = clean
        {
            std::ofstream f(tmp.filePath("bad.json").toStdString());
            f << "{\"format\": \"nv_repro_profile\", \"base\": {\"mode\": \"auto\"}}";
        }
        win->load_profile(tmp.filePath("bad.json"));
        QCOMPARE(win->profile_sig(), sig);
        QVERIFY(win->msg->text().startsWith("cannot load profile: profile lacks"));
    }

    void presetSaveLoad() {   // "preset save/load" / "reset filter"
        win->state->set_param("noise_Cr", PyObj::real(150.0));
        const QString p = tmp.filePath("x.nvpreset.json");
        win->save_preset(p);
        QCOMPARE(win->filter_panel->name->currentText(), QString("x"));
        win->reset_filter();
        QCOMPARE(win->state->params.get("noise_Cr").as_double(), 100.0);
        win->load_preset(p);
        QCOMPARE(win->state->params.get("noise_Cr").as_double(), 150.0);
        win->state->undo->undo();
        QCOMPARE(win->state->params.get("noise_Cr").as_double(), 100.0);
    }

    void beginnerAdvanced() {   // "(h) Beginner / Advanced Mode hides rows and keeps values"
        win->set_advanced(false);
        QVERIFY(win->profile_panel->curve->isHidden() || win->profile_panel->curve->parentWidget()->isHidden());
        QCOMPARE(win->actions["beginner"]->isChecked(), true);
        win->set_advanced(true);
        QVERIFY(!win->profile_panel->curve->parentWidget()->isHidden());
    }

    void profileCheckToggle() {   // "R1 Begin: Assist Image ... Finish (toolbar) / menu toggle / Adjust page"
        win->tabs->setCurrentIndex(0);
        win->set_profile_check(true);
        QVERIFY(win->check_btn->isChecked());
        QVERIFY(win->actions["p_check"]->isChecked());
        QVERIFY(wait_until([&] { return win->viewer->showing_pcheck() && idle(); }));
        QCOMPARE(win->viewer->source(), std::string("pcheck"));
        win->tabs->setCurrentIndex(1);            // Profile Check belongs to the Prepare page
        QVERIFY(!win->pcheck);
        QVERIFY(!win->check_btn->isChecked());
        win->tabs->setCurrentIndex(0);
        win->actions["p_check"]->trigger();        // the menu toggle
        QVERIFY(win->pcheck);
        win->reset_profile();                      // no profile: ends
        QVERIFY(!win->pcheck);
        QVERIFY(!win->actions["p_check"]->isEnabled());
        win->state->undo->undo();
        QVERIFY(win->state->profile.has_profile());
    }

    void adjustPreviewFiltered() {   // "preview filtered"
        win->tabs->setCurrentIndex(1);
        QVERIFY(wait_until([&] { return idle() && win->viewer->showing_filtered(); }));
        QCOMPARE(win->viewer->source(), std::string("filt"));
        QVERIFY(win->msg->text().endsWith("filtered"));
    }

    void holdShowsOriginal() {   // "(e) press-and-hold shows the original ('Filtered' label off)"
        const QPoint c = win->viewer->image_to_widget(320, 180).toPoint();
        QTest::mousePress(win->viewer, Qt::LeftButton, Qt::NoModifier, c);
        QVERIFY(win->viewer->hold());
        QCOMPARE(win->viewer->source(), std::string("orig"));
        QTest::mouseRelease(win->viewer, Qt::LeftButton, Qt::NoModifier, c);
        QCOMPARE(win->viewer->source(), std::string("filt"));
    }

    void previewArea() {   // "Preview Area (crop) filtering" / "preview area off -> whole frame"
        win->area_btn->setChecked(true);
        QVERIFY(win->preview_sel.has_value());
        QCOMPARE(win->preview_sel->h, 180);                 // 50% of the frame, even coordinates
        QCOMPARE(win->preview_sel->w, 320);
        QVERIFY(win->crop().has_value());
        QVERIFY(wait_until([&] { return idle() && win->viewer->showing_filtered(); }));
        win->area_btn->setChecked(false);
        QVERIFY(!win->crop().has_value());
    }

    void thumbnails() {   // "(g) thumbnails: blue = current, orange = Radius window, click = go to frame"
        QVERIFY(!win->thumb(0).isNull());
        QCOMPARE(win->timeline->thumbs->radius, 2);         // Adjust page with the temporal filter on
        win->tabs->setCurrentIndex(0);
        QCOMPARE(win->timeline->thumbs->radius, 0);         // genuine Prepare: blue only
        const int t = win->timeline->frame();
        win->timeline->set_frame(t + 1, true);
        QCOMPARE(win->timeline->frame(), t + 1);
        win->timeline->set_frame(t, true);
    }

    void mostNoisyFrame() {   // "Auto Profile (Most Noisy Frame)"
        win->auto_profile(true);
        QVERIFY(wait_until([&] { return aft_done(); }));
        const int nf = noisiest_frame(win->frames, nullptr);
        QCOMPARE(static_cast<int>(win->state->profile.base.get("ui_frame").as_int()), nf);
        QCOMPARE(win->timeline->frame(), nf);
    }

    void zoomSteps() {
        win->viewer->set_zoom(0);
        const double s = win->viewer->scale_percent();
        win->viewer->zoom_step(1);
        double nxt = 800;
        for (double z : ZOOMS)
            if (z > s * 1.001) {
                nxt = z;
                break;
            }
        QCOMPARE(win->viewer->zoom(), nxt);
        win->viewer->set_zoom(100);
        QCOMPARE(win->viewer->scale_percent(), 100.0);
        QVERIFY(win->viewer_bar->zoom->text().startsWith("100%"));
        win->viewer->set_zoom(0);
    }

    void normalIdentityAt50() {   // "display: Normal identity at 50"
        std::vector<float> rgb(6 * 4 * 3);
        for (size_t i = 0; i < rgb.size(); ++i) rgb[i] = static_cast<float>((i * 37) % 256);
        auto [mn, mx] = minmax(rgb);
        const auto out = render_simple(0, rgb, 4, 6, 50, mn, mx);
        for (size_t i = 0; i < rgb.size(); ++i) QCOMPARE(static_cast<float>(out[i]), rgb[i]);
    }

    void aboutLgpl() {   // About: the Qt LGPL notice
        QWidget* box = win->about();
        auto* mb = qobject_cast<QMessageBox*>(box);
        QVERIFY(mb);
        QVERIFY(mb->text().contains("LGPL"));
        QVERIFY(mb->text().contains(qVersion()));
        box->close();
        QVERIFY(win->actions.count("about_qt"));
    }

    // ================================================================ N3b: Adjust page, Noise Level Check, export
    // helpers
    std::vector<uint8_t> expected_u8(int c, const PyObj& P, const PyObj& prof) {
        // backend.process_frame on the preview's frames, written as the 8-bit export does (clip(rint))
        const int R = static_cast<int>(P.get("radius").as_int());
        auto [idx, wv] = win->engine->window(c, R, std::nullopt);
        const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), c) - idx.begin());
        Planes tmp, rv;
        temporal(wv, ci, prof, P, nullptr, &tmp, &rv);
        const Planes o = finish(wv[static_cast<size_t>(ci)], tmp, rv, prof, P);
        std::vector<uint8_t> out;
        for (int k = 0; k < 3; ++k)
            for (float v : (k == 0 ? o.Y : k == 1 ? o.Cb : o.Cr)) {
                double r = std::nearbyint(v);
                r = std::min(std::max(r, 0.0), 255.0);
                out.push_back(static_cast<uint8_t>(r));
            }
        return out;
    }
    std::vector<std::vector<uint8_t>> decode(const QString& path) {
        std::vector<std::vector<uint8_t>> fr;
        read_frames(path.toStdString(), win->frames[0].W, win->frames[0].H, 0, -1, "", [&](int, Frame&& f) {
            std::vector<uint8_t> b = f.Y;
            b.insert(b.end(), f.Cb.begin(), f.Cb.end());
            b.insert(b.end(), f.Cr.begin(), f.Cr.end());
            fr.push_back(std::move(b));
            return true;
        });
        return fr;
    }
    bool nlc_synced(bool on) {
        FilterPanel* fp = win->filter_panel;
        return win->nlc == on && fp->nl_check->button->isChecked() == on &&
               fp->nl_check->button->text() == (on ? "Finish" : "Begin") &&
               fp->nl_check->title->text().contains(ACTIVE_TITLE) == on && win->fsc_nlc->isChecked() == on &&
               win->actions["nlc"]->isChecked() == on && win->fsc_btn->isChecked() == on &&
               win->fsc_btn->text() == (on ? "Noise Level Check" : "Filter Settings Check");
    }
    bool nlc_shown() {
        return wait_until([&] { return idle() && win->viewer->showing_nlc() && win->nlc_ready(); }, 120);
    }
    std::vector<float> expected_nlc(float* mean = nullptr) {
        const int t = win->timeline->frame();
        const PyObj P = win->state->backend();
        const std::vector<float> m = win->engine->mask(t, *win->state->effective(), win->prof_token, P);
        if (mean) *mean = static_cast<float>(mean_f32(m));
        const RgbImage r = rot90(nlc_image(win->frame_rgb(t), m, win->frames[0].H, win->frames[0].W), win->frames[0].H,
                                 win->frames[0].W, (win->info->rotation / 90) % 4);
        return r.d;
    }
    std::vector<float> shown_nlc() { return win->viewer->image("nlc").d; }   // the mask image (float RGB)
    // viewer.to_qimage(np.clip(rgb).astype(uint8)) as RGB888 bytes (what pane_pixels gives at brightness 50)
    static std::vector<uint8_t> rgb888(const std::vector<float>& rgb, int H, int W) {
        const QImage qi = to_qimage_f(rgb, H, W).convertToFormat(QImage::Format_RGB888);
        std::vector<uint8_t> b(static_cast<size_t>(H) * W * 3);
        for (int y = 0; y < H; ++y) std::memcpy(b.data() + static_cast<size_t>(y) * W * 3, qi.constScanLine(y), static_cast<size_t>(W) * 3);
        return b;
    }

    void filterPanelLayout() {   // tabs Temporal | Spatial | General, the genuine sections and the component / band order
        win->tabs->setCurrentIndex(1);
        FilterPanel* fp = win->filter_panel;
        QStringList titles;
        for (int i = 0; i < fp->tabs->count(); ++i) titles << fp->tabs->tabText(i);
        QCOMPARE(titles, (QStringList{"Temporal", "Spatial", "General"}));
        for (const char* h : {"Temporal Filter", "Temporal Noise", "Noise Reduction Amount", "Spatial Filter", "Noise Level",
                              "Mix with Original", "Performance", "Per-component bands (repro)", "Advanced (repro)"})
            QVERIFY2(fp->headers.count(h), h);
        // Noise Reduction Amount: Luminance -> Cr -> Cb (the "Chrominance" sub-header), Frequencies High .. Ultra Low
        QCOMPARE(fp->row("amt_Y")->label->text(), QString("Luminance"));
        QCOMPARE(fp->row("amt_Cr")->label->text(), QString("Cr"));
        QCOMPARE(fp->row("amt_Cb")->label->text(), QString("Cb"));
        QCOMPARE(static_cast<int>(fp->common.size()), 10);                   // bamt x 5 + bnoise x 5
        QCOMPARE(fp->common[0]->keys, (std::vector<std::string>{"bamt_Y_high", "bamt_Cr_high", "bamt_Cb_high"}));
        QCOMPARE(fp->common[4]->keys[0], std::string("bamt_Y_ultra-low"));
        QCOMPARE(fp->common[5]->keys[0], std::string("bnoise_Y_high"));
        QCOMPARE(fp->icons->open->toolTip(), QString::fromUtf8("Load filter settings from a file…"));
    }

    void defaultValuesShown() {   // the rows show nv.V5_DEFAULTS in UI units
        win->reset_filter();
        FilterPanel* fp = win->filter_panel;
        QCOMPARE(fp->row("amt_Y")->val->text(), QString("70%"));
        QCOMPARE(fp->row("amt_Cr")->val->text(), QString("100%"));
        QCOMPARE(fp->row("t_amount")->val->text(), QString("100%"));
        QCOMPARE(fp->row("noise_Y")->val->text(), QString("+0%"));
        QCOMPARE(fp->row("t_noise")->val->text(), QString("+0%"));
        QCOMPARE(fp->row("t_radius")->val->text(), QString("2"));
        QCOMPARE(fp->row("bamt_Y_ultra-low")->val->text(), QString("off"));
        QCOMPARE(fp->row("adv_threshold")->val->text(), QString("4.00"));
        QCOMPARE(fp->common[4]->val->text(), QString("off"));             // Frequencies Ultra Low: 0% in Y, Cr, Cb
        QVERIFY(fp->check("t_enable")->isChecked());
        QVERIFY(!fp->check("mix_enable")->isChecked());
        QCOMPARE(fp->row("mix")->val->text(), QString("75%"));            // disabled: the value Enable starts with
        QVERIFY(!fp->row("mix")->isEnabled());
        QVERIFY(fp->row("mix")->reset->isHidden());
        QCOMPARE(fp->row("sharp_high")->val->text(), QString("n/a"));
        QVERIFY(!fp->row("sharp_high")->isEnabled());
        QCOMPARE(fp->choice("t_quality")->label->text(), QString("Quality Mode (n/a)"));
    }

    void sliderDragMergesUndo() {   // a drag = one undo step (D6); a new press starts another
        FilterPanel* fp = win->filter_panel;
        Row* r = fp->row("amt_Y");
        const int i0 = win->state->undo->index();
        emit r->slider->sliderPressed();                                   // editStarted -> new_edit
        r->slider->setValue(60);
        r->slider->setValue(55);
        QCOMPARE(win->state->params.get("amt_Y").as_double(), 55.0);
        QCOMPARE(win->state->undo->index(), i0 + 1);
        QCOMPARE(r->val->text(), QString("55%"));
        QVERIFY(!r->reset->isHidden());
        emit r->slider->sliderPressed();
        r->slider->setValue(40);
        QCOMPARE(win->state->undo->index(), i0 + 2);
        win->state->undo->undo();
        QCOMPARE(win->state->params.get("amt_Y").as_double(), 55.0);     // "undo restores value and widget"
        QCOMPARE(r->val->text(), QString("55%"));
        win->state->undo->redo();
        QCOMPARE(r->val->text(), QString("40%"));
        win->state->undo->undo();
        win->state->undo->undo();
        QCOMPARE(r->val->text(), QString("70%"));
        QVERIFY(r->reset->isHidden());
    }

    void frequenciesRowWritesThree() {   // "(b) Frequencies row writes Y / Cr / Cb in one undo step", "mixed"
        FilterPanel* fp = win->filter_panel;
        Row* w = fp->common[1];                                            // bamt Mid
        QCOMPARE(w->keys, (std::vector<std::string>{"bamt_Y_medium", "bamt_Cr_medium", "bamt_Cb_medium"}));
        const int i0 = win->state->undo->index();
        emit w->slider->sliderPressed();
        w->slider->setValue(55);
        QCOMPARE(win->state->undo->index(), i0 + 1);
        for (const auto& k : w->keys) QCOMPARE(win->state->params.get(k).as_double(), 55.0);
        QCOMPARE(fp->row("bamt_Cr_medium")->val->text(), QString("55%"));  // the per-component rows follow
        win->state->new_edit();
        win->state->set_param("bamt_Cr_medium", PyObj::real(30.0));
        QCOMPARE(w->val->text(), QString("mixed"));
        QVERIFY(!w->reset->isHidden());
        win->state->undo->undo();
        QCOMPARE(w->val->text(), QString("55%"));
        win->state->undo->undo();
        QCOMPARE(w->val->text(), QString("100%"));
    }

    void mixEnableStartsAt25() {   // "(c) Enable Mix with mix 0 starts at 75% (backend 25), one undo step"
        FilterPanel* fp = win->filter_panel;
        const int i0 = win->state->undo->index();
        fp->check("mix_enable")->click();
        QCOMPARE(win->state->params.get("mix").as_double(), 25.0);
        QVERIFY(win->state->params.get("mix_enable").truthy());
        QCOMPARE(win->state->undo->index(), i0 + 1);
        QVERIFY(fp->row("mix")->isEnabled());
        QCOMPARE(fp->row("mix")->val->text(), QString("75%"));
        QCOMPARE(win->state->backend().get("mix").as_double(), 0.25);
        fp->check("mix_enable")->click();
        QVERIFY(!win->state->params.get("mix_enable").truthy());
        QCOMPARE(win->state->params.get("mix").as_double(), 25.0);         // the value is kept, the backend gets 0
        QCOMPARE(win->state->backend().get("mix").as_double(), 0.0);
        QVERIFY(!fp->row("mix")->isEnabled());
        win->state->undo->undo();
        win->state->undo->undo();
        QCOMPARE(win->state->params.get("mix").as_double(), 0.0);
    }

    void presetDirtyStarAndReset() {   // "default *" after an edit, Reset Filter Settings clears it
        FilterPanel* fp = win->filter_panel;
        QCOMPARE(fp->name->currentText(), QString("default"));
        win->state->new_edit();
        win->state->set_param("noise_Cr", PyObj::real(150.0));
        QCOMPARE(fp->name->currentText(), QString("default *"));
        QCOMPARE(fp->row("noise_Cr")->val->text(), QString("+50%"));
        win->reset_filter();
        QCOMPARE(fp->name->currentText(), QString("default"));
        QCOMPARE(fp->row("noise_Cr")->val->text(), QString("+0%"));
    }

    void resetSpatialOnlySpatialKeys() {   // ↺ of the Spatial Filter header
        win->state->new_edit();
        win->state->set_param("amt_Y", PyObj::real(30.0));
        win->state->new_edit();
        win->state->set_param("t_amount", PyObj::real(50.0));
        win->state->new_edit();
        win->state->set_param("adv_threshold", PyObj::real(3.0));
        const int i0 = win->state->undo->index();
        win->reset_spatial();
        QCOMPARE(win->state->undo->index(), i0 + 1);
        QCOMPARE(win->state->params.get("amt_Y").as_double(), 70.0);
        QCOMPARE(win->state->params.get("t_amount").as_double(), 50.0);    // temporal: untouched
        QCOMPARE(win->state->params.get("adv_threshold").as_double(), 3.0);   // not a SPATIAL_PREFIXES key
        QCOMPARE(win->state->undo->undoText(), QString("Reset Spatial Settings"));
        win->reset_filter();
    }

    void beginnerHidesAdvancedRows() {   // "(h) Beginner / Advanced Mode hides rows and keeps values"
        FilterPanel* fp = win->filter_panel;
        win->state->new_edit();
        win->state->set_param("t_noise", PyObj::real(60.0));
        win->set_advanced(false);
        QVERIFY(fp->headers.at("Temporal Noise")->isHidden());
        QVERIFY(fp->row("noise_Cr")->isHidden());
        QVERIFY(!fp->row("amt_Y")->isHidden());
        QCOMPARE(fp->sharp_rows[0]->label->text(), QString("Fine"));
        QVERIFY(fp->mode_lbl->text().contains("Beginner Mode"));
        QCOMPARE(win->state->params.get("t_noise").as_double(), 60.0);
        win->set_advanced(true);
        QVERIFY(!fp->headers.at("Temporal Noise")->isHidden());
        QCOMPARE(fp->sharp_rows[0]->label->text(), QString("High"));
        QCOMPARE(fp->row("t_noise")->val->text(), QString("-40%"));
        win->state->undo->undo();
    }

    void bandAvailability420() {   // "4:2:0 chroma has 4 levels: Cb / Cr High unused, Y High used"
        FilterPanel* fp = win->filter_panel;
        QVERIFY(fp->row("bamt_Y_high")->isEnabled());
        QVERIFY(!fp->row("bamt_Cr_high")->isEnabled());
        QVERIFY(!fp->row("bnoise_Cb_high")->isEnabled());
        QVERIFY(fp->row("bamt_Cr_medium")->isEnabled());
        QVERIFY(fp->common[0]->isEnabled());                                // the common High row: Y uses it
        QVERIFY(fp->row("bamt_Cr_high")->slider->toolTip().startsWith("Not used with this profile: Cr has 4"));
    }

    void performanceTable() {   // the General tab's Performance box follows the settings
        FilterPanel* fp = win->filter_panel;
        QString t = fp->perf->text();
        QVERIFY(t.contains("Frame Size:&nbsp;&nbsp;</td><td>640x360 progressive"));
        QVERIFY(t.contains("Frame Bitdepth:&nbsp;&nbsp;</td><td>8-bit (yuv420p)"));
        QVERIFY(t.contains("Radius:&nbsp;&nbsp;</td><td>2 frames"));
        QVERIFY(t.contains("Frequencies:&nbsp;&nbsp;</td><td>High, Mid, Low, Very Low"));
        QVERIFY(t.contains("Temporal Filter:&nbsp;&nbsp;</td><td>Enabled"));
        win->state->new_edit();
        win->state->set_param("t_enable", PyObj::boolean(false));
        QVERIFY(fp->perf->text().contains("Temporal Filter:&nbsp;&nbsp;</td><td>Disabled"));
        QCOMPARE(win->timeline->thumbs->radius, 0);                        // no temporal window on the thumbnails
        win->state->undo->undo();
        QCOMPARE(win->timeline->thumbs->radius, 2);
    }

    void checkSpeed() {   // Check Speed: frames/sec of one frame
        win->check_speed();
        QCOMPARE(win->filter_panel->speed->text(), QString::fromUtf8("measuring…"));
        QVERIFY(wait_until([&] { return idle(); }));
        QVERIFY(QRegularExpression("^\\d+\\.\\d\\d frames/sec \\(1 frame, 1 thread\\)$").match(win->filter_panel->speed->text()).hasMatch());
        QVERIFY(win->speed_result > 0);
    }

    void performanceTabAction() {
        win->tabs->setCurrentIndex(0);
        win->filter_panel->tabs->setCurrentIndex(0);
        win->actions["perf"]->trigger();
        QCOMPARE(win->tabs->currentIndex(), 1);
        QCOMPARE(win->filter_panel->tabs->currentIndex(), 2);
        win->filter_panel->tabs->setCurrentIndex(0);
    }

    void menusEnabledForN3b() {   // export / Performance tab / Noise Level Check are live (N3c: also Preferences, Batch, Profile Viewer)
        QVERIFY(win->actions["export"]->isEnabled());
        QVERIFY(win->actions["perf"]->isEnabled());
        QVERIFY(win->actions["nlc"]->isEnabled());                          // with a profile
        QVERIFY(win->fsc_nlc->isEnabled());
        QVERIFY(win->actions["prefs"]->isEnabled());
        QVERIFY(win->actions["batch"]->isEnabled());
        QVERIFY(win->actions["p_viewer"]->isEnabled());
    }

    void tNoiseScalesTemporalOnly() {   // "NLC (c) Temporal Noise Level +0% = before; -40% scales the temporal noise level only"
        const int t = win->timeline->frame();
        const PyObj prof = *win->state->effective();
        auto [idx, wv] = win->engine->window(t, 2, std::nullopt);
        const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), t) - idx.begin());
        PyObj ui = default_params();
        ui.set("t_noise", PyObj::real(60.0));
        const PyObj Pn = backend_params(ui), P0 = backend_params(default_params());
        QCOMPARE(Pn.get("t_noise").as_double(), 0.6);
        Planes a, ra, b, rb, c, rc;
        temporal(wv, ci, prof, Pn, nullptr, &a, &ra);                       // t_noise 0.6
        PyObj Q = Pn;
        Q.set("noise_level", PyObj::real(Pn.get("noise_level").as_double() * 0.6));
        Q.set("t_noise", PyObj::real(1.0));
        temporal(wv, ci, prof, Q, nullptr, &b, &rb);                        // = noise_level x 0.6, t_noise 1
        temporal(wv, ci, prof, P0, nullptr, &c, &rc);
        QVERIFY(a.Y == b.Y && ra.Y == rb.Y);
        QVERIFY(!(a.Y == c.Y));
        // the preview equals process_frame with the same parameters
        const auto pr = win->engine->process(t, prof, win->prof_token, P0, std::nullopt).first;
        Planes tmp, rv;
        temporal(wv, ci, prof, P0, nullptr, &tmp, &rv);
        const Planes o = finish(wv[static_cast<size_t>(ci)], tmp, rv, prof, P0);
        QVERIFY(pr == win->engine->to_rgb(clip_planes(o)));
    }

    void temporalMaskEdges() {   // "NLC (b) mask = 1 on a repeated frame, 0 at Amount 0 / one frame"
        const int t = win->timeline->frame();
        const PyObj prof = *win->state->effective();
        const PyObj P = win->state->backend();
        const Planes pl = win->engine->crop_planes(win->frames[static_cast<size_t>(t)], std::nullopt);
        std::vector<Planes> same(5, pl);
        Planes tmp, rv;
        temporal(same, 2, prof, P, nullptr, &tmp, &rv);
        std::vector<float> m(rv.Y.size());
        nvc_temporal_mask(rv.Y.data(), rv.H, rv.W, 5, P.get("temporal").as_double(), m.data());
        float mn = 1.0f, mx = 0.0f;
        for (float v : m) {
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
        QVERIFY(mn > 0.999f);
        nvc_temporal_mask(rv.Y.data(), rv.H, rv.W, 1, 1.0, m.data());
        for (float v : m) QCOMPARE(v, 0.0f);
        nvc_temporal_mask(rv.Y.data(), rv.H, rv.W, 5, 0.0, m.data());
        for (float v : m) QCOMPARE(v, 0.0f);
        PyObj ui = default_params();
        ui.set("t_noise", PyObj::real(0.0));
        const std::vector<float> m0 = win->engine->mask(t, prof, win->prof_token, backend_params(ui));
        for (float v : m0) QVERIFY(v < 1e-3f);
    }

    void nlcBeginShowsMask() {   // "NLC (a) Begin: temporal mask image on the primary pane, viewer Normal; Finish restores"
        win->tabs->setCurrentIndex(1);
        win->viewer->set_mode("RGB");
        win->filter_panel->nl_check->button->click();
        QVERIFY(nlc_synced(true));
        QCOMPARE(win->viewer->mode(), std::string("Normal"));
        QVERIFY(!win->filter_panel->tabs->isTabEnabled(1));
        QVERIFY(!win->filter_panel->tabs->isTabEnabled(2));
        QVERIFY(!win->filter_panel->headers.at("Temporal Noise")->body->isHidden());
        QVERIFY(nlc_shown());
        QCOMPARE(win->viewer->source(), std::string("nlc"));
        float mean = 0;
        const std::vector<float> exp = expected_nlc(&mean);
        QVERIFY(shown_nlc() == exp);                                        // vw._f["nlc"] == rot90(nlc_image(..))
        QVERIFY(!exp.empty());
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(mean));
        QCOMPARE(win->msg->text(), QString("Noise Level Check: mask of frame %1 (mean %2)").arg(win->timeline->frame()).arg(buf));
        win->filter_panel->nl_check->button->click();
        QVERIFY(nlc_synced(false));
        QVERIFY(!win->viewer->showing_nlc());
        QCOMPARE(win->viewer->mode(), std::string("RGB"));                 // restored
        QVERIFY(win->filter_panel->tabs->isTabEnabled(1));
        win->viewer->set_mode("Normal");
    }

    void nlcRecomputesOnTemporalSettings() {   // "NLC (b') Noise Level / Radius / Enable recompute the mask, undo shows the cached one"
        win->filter_panel->nl_check->button->click();
        QVERIFY(nlc_shown());
        const auto before = shown_nlc();
        win->state->new_edit();
        win->filter_panel->row("t_noise")->slider->setValue(-60);          // display -60% = 40
        QCOMPARE(win->state->params.get("t_noise").as_double(), 40.0);
        QVERIFY(nlc_shown());
        QVERIFY(!(shown_nlc() == before));
        float m1 = 0;
        expected_nlc(&m1);
        QVERIFY(m1 < 1.0f);
        win->state->new_edit();
        win->state->set_param("t_radius", PyObj::integer(1));
        QVERIFY(nlc_shown());
        const auto r1 = shown_nlc();
        win->state->new_edit();
        win->state->set_param("t_enable", PyObj::boolean(false));
        QVERIFY(nlc_shown());
        const auto e0 = shown_nlc();
        QVERIFY(!(e0 == r1));
        win->state->undo->undo();
        QVERIFY(nlc_shown());
        QVERIFY(shown_nlc() == r1);                                          // from the cache
        win->state->undo->undo();
        win->state->undo->undo();
        QVERIFY(nlc_shown());
        QVERIFY(shown_nlc() == before);
        QCOMPARE(win->state->params.get("t_noise").as_double(), 100.0);
        // a spatial setting does not change the mask key
        const std::string k = win->nlc_key();
        win->state->new_edit();
        win->state->set_param("amt_Y", PyObj::real(50.0));
        QCOMPARE(win->nlc_key(), k);
        win->state->undo->undo();
    }

    void nlcAnotherFrameDropsImage() {   // "another frame drops the old image"
        QVERIFY(win->nlc);
        const int t = win->timeline->frame();
        const int nxt = t + 1 < static_cast<int>(win->frames.size()) ? t + 1 : t - 1;
        win->timeline->set_frame(nxt, true);
        QVERIFY(!win->viewer->showing_nlc() || win->nlc_ready());           // no stale image of the previous frame
        QVERIFY(nlc_shown());
        QVERIFY(win->nlc_shown_key() && win->nlc_shown_key()->rfind(std::to_string(nxt) + "|", 0) == 0);
        QVERIFY(shown_nlc() == expected_nlc());
        win->timeline->set_frame(t, true);
        QVERIFY(nlc_shown());
    }

    void nlcThreeEntriesAndAssist() {   // "NLC (d) one state for the panel, the toolbar menu and Tools; exclusive with Assist"
        win->actions["nlc"]->trigger();                                      // off
        QVERIFY(nlc_synced(false));
        win->fsc_nlc->trigger();                                             // on
        QVERIFY(nlc_synced(true));
        win->fsc_btn->toggle();                                              // the button only shows the state
        QVERIFY(nlc_synced(true));
        win->set_assist(true);
        QVERIFY(nlc_synced(false));
        QVERIFY(win->viewer->assist() && win->filter_panel->assist->button->isChecked());
        win->fsc_nlc->trigger();
        QVERIFY(nlc_synced(true));
        QVERIFY(!win->viewer->assist() && !win->filter_panel->assist->button->isChecked());
        QList<QAction*> acts = win->fsc_btn->menu()->actions();
        acts.last()->trigger();                                              // &Finish
        QVERIFY(nlc_synced(false));
        QVERIFY(!win->viewer->assist());
    }

    void nlcEndsOnPrepareAndWithoutProfile() {   // "ends on the Prepare page / Reset Profile"
        win->fsc_nlc->trigger();
        QVERIFY(nlc_synced(true));
        win->tabs->setCurrentIndex(0);
        QVERIFY(nlc_synced(false));
        win->actions["nlc"]->trigger();                                      // switches to the Adjust page
        QCOMPARE(win->tabs->currentIndex(), 1);
        QVERIFY(nlc_synced(true));
        win->reset_profile();
        QVERIFY(nlc_synced(false));
        QVERIFY(!win->filter_panel->nl_check->isEnabled() && !win->fsc_nlc->isEnabled() && !win->actions["nlc"]->isEnabled());
        win->state->undo->undo();
        QVERIFY(win->filter_panel->nl_check->isEnabled());
        QVERIFY(wait_until([&] { return idle() && win->viewer->showing_filtered(); }));
    }

    void exportFilterDefaults() { QCOMPARE(win->export_filter(), std::make_pair(QString("MP4 (*.mp4)"), QString(".mp4"))); }

    void exportLosslessEqualsProcessFrame() {   // "export (CRF 0) == backend.process_frame bit for bit" (+ the side files)
        win->tabs->setCurrentIndex(1);
        win->state->new_edit();
        win->state->set_param("bamt_Cr_medium", PyObj::real(60.0));      // non-uniform channels exercise the per-channel path
        win->prefs["crf"] = 0;
        const QString dst = tmp.filePath("export.mp4");
        const int lo = 100, n = 4;
        win->process_video(dst, lo, n);
        QVERIFY(win->export_ != nullptr);
        QVERIFY(!win->actions["export"]->isEnabled());
        win->process_video(dst, lo, n);
        QCOMPARE(win->msg->text(), QString("an export is already running"));
        QVERIFY(wait_until([&] { return win->export_ == nullptr; }, 1800));
        QVERIFY(win->export_ok);
        QVERIFY(win->actions["export"]->isEnabled() && win->b_apply->isEnabled());
        QCOMPARE(win->msg->text(), "wrote " + dst);
        const PyObj pj = PyObj::parse([&] { std::ifstream f(tmp.filePath("export.ui_params.json").toStdString()); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }());
        QVERIFY(pj == win->state->backend());
        QCOMPARE(pj.get("ui_channels").get("Cr").get("bands").get("medium").as_double(), 0.6);
        const PyObj prof = PyObj::parse([&] { std::ifstream f(tmp.filePath("export.ui_profile.json").toStdString()); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }());
        QVERIFY(prof == *win->state->effective());
        QVERIFY(QFileInfo::exists(tmp.filePath("export.json")));
        const auto got = decode(dst);
        QCOMPARE(static_cast<int>(got.size()), n);
        const PyObj P = win->state->backend();
        for (int k = 0; k < n; ++k) QVERIFY2(got[static_cast<size_t>(k)] == expected_u8(lo + k, P, prof), qPrintable(QString("frame %1").arg(lo + k)));
        win->state->undo->undo();
    }

    void exportTNoiseDiffersFromDefault() {   // "NLC (c) export with Temporal Noise Level -40% == process_frame, differs from +0%"
        win->state->new_edit();
        win->state->set_param("t_noise", PyObj::real(60.0));
        const QString dst = tmp.filePath("export_tnoise.mp4");
        const int lo = 100, n = 2;
        win->process_video(dst, lo, n);
        QVERIFY(wait_until([&] { return win->export_ == nullptr; }, 1800) && win->export_ok);
        const auto got = decode(dst);
        QCOMPARE(static_cast<int>(got.size()), n);
        const PyObj P = win->state->backend(), prof = *win->state->effective();
        QCOMPARE(P.get("t_noise").as_double(), 0.6);
        PyObj P1 = P;
        P1.set("t_noise", PyObj::real(1.0));
        bool differs = false;
        for (int k = 0; k < n; ++k) {
            QVERIFY(got[static_cast<size_t>(k)] == expected_u8(lo + k, P, prof));
            differs = differs || got[static_cast<size_t>(k)] != expected_u8(lo + k, P1, prof);
        }
        QVERIFY(differs);
        win->state->undo->undo();
        QCOMPARE(win->state->params.get("t_noise").as_double(), 100.0);
    }

    void exportCancelRemovesFiles() {   // Cancel export: the partial output and the side files are removed
        win->prefs["crf"] = 16;
        const QString dst = tmp.filePath("export_cancel.mp4");
        win->process_video(dst, 0, 150);
        QVERIFY(win->export_ != nullptr);
        QVERIFY(win->cancel_btn->isVisible() || !win->cancel_btn->isHidden());
        QVERIFY(wait_until([&] { return win->prog->value() > 0 || !win->export_; }, 120));
        win->cancel_export();
        QVERIFY(wait_until([&] { return win->export_ == nullptr; }, 120));
        QVERIFY(!win->export_ok);
        QCOMPARE(win->msg->text(), QString("export cancelled"));
        for (const char* f : {"export_cancel.mp4", "export_cancel.json", "export_cancel.ui_params.json", "export_cancel.ui_profile.json"})
            QVERIFY2(!QFileInfo::exists(tmp.filePath(f)), f);
        QVERIFY(win->actions["export"]->isEnabled());
    }

    void exportWithoutProfileOrVideo() {
        win->reset_profile();
        win->process_video(tmp.filePath("none.mp4"), 0, 1);
        QCOMPARE(win->msg->text(), QString("no profile: run Auto Profile first"));
        QVERIFY(win->export_ == nullptr);
        win->state->undo->undo();
    }

    void presetProfileFilesFromPython() {   // files the PySide6 UI wrote (qtui/tests/fixtures, tracked by git) load and round-trip
        const QString dir = QString(NV_SOURCE_ROOT) + "/qtui/tests/fixtures";
        QVERIFY2(QFileInfo::exists(dir + "/py_preset.nvpreset.json"), "qtui/tests/fixtures/py_preset.nvpreset.json is missing");
        auto text = [](const QString& p) { std::ifstream f(p.toStdString(), std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); };
        win->load_preset(dir + "/py_preset.nvpreset.json");
        QVERIFY(win->msg->text().startsWith("preset loaded"));
        QCOMPARE(win->filter_panel->name->currentText(), QString("py_preset"));
        QCOMPARE(preset_json(win->state->params).dump(1), text(dir + "/py_preset.nvpreset.json"));
        QCOMPARE(win->state->params.get("t_noise").as_double(), 60.0);
        QCOMPARE(win->state->params.get("t_radius").as_int(), 1);
        QCOMPARE(win->filter_panel->row("t_noise")->val->text(), QString("-40%"));
        win->save_preset(tmp.filePath("rt.nvpreset.json"));
        QCOMPARE(text(tmp.filePath("rt.nvpreset.json")), text(dir + "/py_preset.nvpreset.json"));
        win->load_profile(dir + "/py_profile.nvprof.json");
        QVERIFY(win->msg->text().startsWith("profile loaded"));
        QCOMPARE(win->state->profile.to_json().dump(1), text(dir + "/py_profile.nvprof.json"));
        win->save_profile(tmp.filePath("rt.nvprof.json"));
        QCOMPARE(text(tmp.filePath("rt.nvprof.json")), text(dir + "/py_profile.nvprof.json"));
        win->state->undo->undo();
        win->state->undo->undo();
        win->reset_filter();
    }

    void applyButtonRefreshesPreview() {
        win->tabs->setCurrentIndex(1);
        const long g = win->gen;
        win->b_apply->click();
        QCOMPARE(win->msg->text(), QString("filter settings applied to preview"));
        QVERIFY(win->gen > g);
        QVERIFY(wait_until([&] { return idle() && win->viewer->showing_filtered(); }));
    }

    void autoCompleteThroughCore() {   // M1: auto_complete = nvc_interp (numpy arr_interp) on the edited bins
        Factors f = unit_factors();
        std::array<std::set<int>, 3> pins;
        f[1][1] = 0.4;
        f[1][7] = 2.8;
        pins[1] = {1, 7};
        const Factors g = auto_complete(f, pins);
        const double xp[2] = {32.0, 224.0}, fp[2] = {0.4, 2.8};
        for (int i = 0; i < 9; ++i) {
            double x = 32.0 * i, r;
            nvc_interp(xp, fp, 2, &x, 1, &r);
            QCOMPARE(g[1][i], (i == 1 || i == 7) ? f[1][i] : r);
        }
        QCOMPARE(g[1][0], 0.4);
        QCOMPARE(g[1][8], 2.8);
        QCOMPARE(g[0], unit_factors()[0]);
    }

    void thumbnailWindowFollowsRadius() {   // orange thumbnails = the temporal window on the Adjust page
        win->tabs->setCurrentIndex(1);
        QCOMPARE(win->timeline->thumbs->radius, 2);
        win->state->new_edit();
        win->state->set_param("t_radius", PyObj::integer(1));
        QCOMPARE(win->timeline->thumbs->radius, 1);
        QVERIFY(win->timeline->thumbs->frame_color(win->timeline->frame() - 1).has_value());
        QVERIFY(!win->timeline->thumbs->frame_color(win->timeline->frame() - 2).has_value());
        win->tabs->setCurrentIndex(0);
        QCOMPARE(win->timeline->thumbs->radius, 0);                        // genuine Prepare: blue only
        win->tabs->setCurrentIndex(1);
        win->state->undo->undo();
        QCOMPARE(win->timeline->thumbs->radius, 2);
    }

    void exactValueEntryAndReset() {   // a click on the value opens the exact entry; ⋈ resets to the default
        Row* r = win->filter_panel->row("noise_Cb");
        const int i0 = win->state->undo->index();
        emit r->val->clicked();
        QCOMPARE(r->stack->currentIndex(), 1);
        r->spin->setValue(37.0);                                             // display +37%
        emit r->spin->editingFinished();
        QCOMPARE(r->stack->currentIndex(), 0);
        QCOMPARE(win->state->params.get("noise_Cb").as_double(), 137.0);
        QCOMPARE(r->val->text(), QString("+37%"));
        QCOMPARE(win->state->undo->index(), i0 + 1);
        QVERIFY(!r->reset->isHidden());
        r->reset->click();
        QCOMPARE(win->state->params.get("noise_Cb").as_double(), 100.0);
        QCOMPARE(win->state->undo->index(), i0 + 2);
        QVERIFY(r->reset->isHidden());
        win->state->undo->undo();
        win->state->undo->undo();
        QCOMPARE(win->state->undo->index(), i0);
    }

    void choiceAndEnableRows() {   // the combo box and the Enable boxes drive the backend parameters
        ChoiceRow* c = win->filter_panel->choice("adv_similarity");
        c->box->setCurrentIndex(1);
        QCOMPARE(win->state->params.get("adv_similarity").as_str(), std::string("pyramid"));
        QCOMPARE(win->state->backend().get("similarity").as_str(), std::string("pyramid"));
        QCOMPARE(win->state->undo->undoText(), QString("Change Temporal similarity"));
        win->state->undo->undo();
        QCOMPARE(c->box->currentIndex(), 0);
        EnableCheck* e = win->filter_panel->check("s_enable");
        e->click();
        QVERIFY(!win->state->params.get("s_enable").truthy());
        QVERIFY(!win->state->backend().get("ui_spatial").truthy());
        QVERIFY(win->filter_panel->perf->text().contains("Spatial Filter:&nbsp;&nbsp;</td><td>Disabled"));
        // spatial off: the preview is the temporal result (finish returns tmp)
        const int t = win->timeline->frame();
        const PyObj P = win->state->backend(), prof = *win->state->effective();
        auto [idx, wv] = win->engine->window(t, 2, std::nullopt);
        const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), t) - idx.begin());
        Planes tmp, rv;
        temporal(wv, ci, prof, P, nullptr, &tmp, &rv);
        QVERIFY(finish(wv[static_cast<size_t>(ci)], tmp, rv, prof, P).Y == tmp.Y);
        win->state->undo->undo();
        QVERIFY(e->isChecked());
    }

    void undoHistoryMenu() {   // the ▾ of the panel's undo button lists the last steps (IconBar.history_)
        win->state->new_edit();
        win->state->set_param("amt_Cb", PyObj::real(80.0));
        QMenu* m = win->filter_panel->icons->undo->menu();
        QVERIFY(m);
        emit m->aboutToShow();
        QVERIFY(!m->actions().isEmpty());
        QCOMPARE(m->actions().first()->text(), QString("Change Cb"));
        const int i = win->state->undo->index();
        m->actions().first()->trigger();                                    // undo up to that entry
        QCOMPARE(win->state->undo->index(), i - 1);
        QCOMPARE(win->state->params.get("amt_Cb").as_double(), 100.0);
        QMenu* rm = win->filter_panel->icons->redo->menu();
        emit rm->aboutToShow();
        QCOMPARE(rm->actions().first()->text(), QString("Change Cb"));
        rm->actions().first()->trigger();
        QCOMPARE(win->state->params.get("amt_Cb").as_double(), 80.0);
        win->state->undo->undo();
    }

    // ================================================================ N3c: Preferences, Profile Viewer, Batch Profiler,
    // Generic Profile, the remaining menu items, the OFX host mode and the Bit depth setting
    // helpers
    QDialog* find_dialog(const QString& title) {
        for (QDialog* d : win->findChildren<QDialog*>())
            if (d->windowTitle() == title && d->isVisible()) return d;
        return nullptr;
    }
    static QString text_of(const QString& p) {
        std::ifstream f(p.toStdString(), std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        return QString::fromStdString(ss.str());
    }
    // a run of the OFX test driver (qtui_ofx_drv next to this binary) with the host frames on stdin
    OfxRun ofx_run(const QStringList& args, const QByteArray& in, const QString& script, int timeout_ms = 600000) {
        QProcess p;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("QT_QPA_PLATFORM", "offscreen");
        env.insert("NV_OFX_SCRIPT", script);
        env.insert("NV_UI_QT_SETTINGS", tmp.filePath("ofx.ini"));
        p.setProcessEnvironment(env);
        p.start(QCoreApplication::applicationDirPath() + "/qtui_ofx_drv", QStringList{"--ofx"} + args);
        if (!p.waitForStarted(10000)) return {};
        p.write(in);
        p.closeWriteChannel();
        if (!p.waitForFinished(timeout_ms)) {
            p.kill();
            p.waitForFinished(5000);
            return {-2, p.readAllStandardOutput(), p.readAllStandardError()};
        }
        return {p.exitCode(), p.readAllStandardOutput(), p.readAllStandardError()};
    }
    // n frames of the preview (8-bit test.mp4) from frame lo in the OFX layout at depth 8 / 16 / 32
    QByteArray ofx_bytes(int lo, int n, int depth) {
        QByteArray b;
        for (int k = 0; k < n; ++k) {
            const Frame& f = win->frames[static_cast<size_t>(lo + k)];
            for (const auto* pl : {&f.Y, &f.Cb, &f.Cr})
                for (size_t i = 0; i < pl->size(); ++i) {
                    const uint8_t v = (*pl)[i];
                    if (depth == 8) {
                        b.append(static_cast<char>(v));
                    } else if (depth == 16) {
                        const uint16_t u = static_cast<uint16_t>(v * 256 + (i * 37 + static_cast<size_t>(k)) % 256);
                        b.append(reinterpret_cast<const char*>(&u), 2);
                    } else {
                        float x = static_cast<float>(v) / 255.0f;
                        if (pl == &f.Y && (i == 1000 || i == 1001)) x = std::nanf("");
                        if (pl == &f.Cb && i == 50) x = -INFINITY;
                        b.append(reinterpret_cast<const char*>(&x), 4);
                    }
                }
        }
        return b;
    }
    // what the OFX mode writes for frame t of the host frames `in` (Auto Profile of t, the given parameters)
    QByteArray ofx_expected(const QByteArray& in, int W, int H, int depth, int t, const PyObj& P) {
        const size_t fs = static_cast<size_t>(ofx_frame_size(W, H, depth));
        std::vector<Frame> fr;
        std::vector<RestoreMap> rs;
        for (size_t o = 0; o + fs <= static_cast<size_t>(in.size()); o += fs) {
            const auto* d = reinterpret_cast<const uint8_t*>(in.constData()) + o;
            RestoreMap rm;
            if (depth == 8) {
                Frame f;
                f.H = H;
                f.W = W;
                f.Hc = H / 2;
                f.Wc = W / 2;
                const size_t ny = static_cast<size_t>(W) * H, nc = static_cast<size_t>(W / 2) * (H / 2);
                f.Y.assign(d, d + ny);
                f.Cb.assign(d + ny, d + ny + nc);
                f.Cr.assign(d + ny + nc, d + ny + 2 * nc);
                fr.push_back(std::move(f));
            } else {
                fr.push_back(frame_from_planes(ofx_unpack(d, W, H, depth, &rm)));
            }
            rs.push_back(std::move(rm));
        }
        const PyObj prof = build_profile(fr, t, false, "bt709", "auto", nullptr, nullptr);
        PreviewEngine eng(&fr, "bt709", false);
        auto [idx, wv] = eng.window(t, static_cast<int>(P.get("radius").as_int()), std::nullopt);
        const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), t) - idx.begin());
        Planes tmp, rv;
        temporal(wv, ci, prof, P, nullptr, &tmp, &rv);
        const Planes o = finish(wv[static_cast<size_t>(ci)], tmp, rv, prof, P);
        const auto b = ofx_pack(o, depth, rs[static_cast<size_t>(t)].empty() ? nullptr : &rs[static_cast<size_t>(t)]);
        return QByteArray(reinterpret_cast<const char*>(b.data()), static_cast<qsizetype>(b.size()));
    }

    void menusAllEnabledN3c() {   // "menus ... Tools Help": every reproduction item of N3a..N3c is live (no "N3c" left)
        for (const char* k : {"export", "perf", "nlc", "prefs", "batch", "p_viewer", "p_generic", "open", "save_frame"})
            QVERIFY2(win->actions[k]->isEnabled(), k);
        QCOMPARE(win->actions["prefs"]->shortcut(), QKeySequence("F10"));
        QCOMPARE(win->actions["p_viewer"]->shortcut(), QKeySequence("Ctrl+Shift+I"));
        std::function<void(QMenu*)> walk = [&](QMenu* m) {
            for (QAction* a : m->actions()) {
                QVERIFY2(!a->toolTip().contains("N3c") && !a->statusTip().contains("N3c"), qPrintable(a->text()));
                if (a->menu()) walk(a->menu());
            }
        };
        for (QAction* top : win->menuBar()->actions())
            if (top->menu()) walk(top->menu());
    }

    void variantsDisabled() {   // "non-implemented controls show 'n/a'": Variants (menu and both toolbars) disabled
        QMenu* vm = win->menuBar()->actions()[4]->menu();
        QCOMPARE(vm->actions().size(), 1);
        QVERIFY(!vm->actions()[0]->isEnabled());
        QCOMPARE(vm->actions()[0]->toolTip(), QString("Not available in the reproduction"));
        int n = 0;
        for (int i = 0; i < 2; ++i)
            for (QToolButton* b : win->tb_stack->widget(i)->findChildren<QToolButton*>(Qt::FindDirectChildrenOnly))
                if (b->text() == "Variants") {
                    QVERIFY(!b->isEnabled());
                    ++n;
                }
        QCOMPARE(n, 2);
    }

    void preferencesDialogRows() {   // Tools > Preferences… (F10): the Python rows, keys and defaults (+ the two C++ rows)
        QDialog* dlg = win->preferences();
        QVERIFY(dlg && dlg->isVisible());
        QCOMPARE(dlg->windowTitle(), QString("Preferences"));
        auto* f = dlg->findChild<QFormLayout*>();
        QStringList labels;
        for (int i = 0; i < f->rowCount(); ++i) {
            QLayoutItem* li = f->itemAt(i, QFormLayout::LabelRole);
            if (li && qobject_cast<QLabel*>(li->widget())) labels << qobject_cast<QLabel*>(li->widget())->text();
        }
        QCOMPARE(labels, (QStringList{"Computing: export processes (jobs)", "Output: x264 CRF", "Output: audio",
                                      "Preview Area (fraction of the frame)", "Preview cache: frames in memory (GB)",
                                      "Bit depth", "ffmpeg (directory or binary)"}));
        QList<QSpinBox*> spins;
        for (QSpinBox* s : dlg->findChildren<QSpinBox*>())
            if (!qobject_cast<QDoubleSpinBox*>(s)) spins << s;
        QCOMPARE(spins.size(), 2);
        const unsigned hc = std::thread::hardware_concurrency();
        QCOMPARE(spins[0]->value(), std::max(1, static_cast<int>(hc ? hc : 2) - 2));    // PREFS["jobs"]
        QCOMPARE(spins[0]->maximum(), 64);
        QCOMPARE(spins[1]->value(), win->prefs["crf"].toInt());
        QCOMPARE(spins[1]->maximum(), 51);
        auto dsp = dlg->findChildren<QDoubleSpinBox*>();
        QCOMPARE(dsp.size(), 2);
        QCOMPARE(dsp[0]->value(), 0.5);
        QCOMPARE(dsp[1]->value(), 0.25);            // the test's prefs/mem_gb 0.05, clamped to the box's 0.25..64 (as in Python)
        QComboBox* depth = nullptr;
        for (QComboBox* c : dlg->findChildren<QComboBox*>())
            if (c->findData("source") >= 0) depth = c;
        QVERIFY(depth);
        QCOMPARE(depth->count(), 6);
        QCOMPARE(depth->currentData().toString(), QString("8"));
        QCOMPARE(depth->itemText(5), QString("32-bit float (RGB)"));
        QStringList checks;
        for (QCheckBox* c : dlg->findChildren<QCheckBox*>()) checks << c->text();
        QCOMPARE(checks, (QStringList{"Auto Profile when a video is opened",
                                      "Use the native core (libnvcore.dylib; same results, faster)",
                                      "Auto Fine-Tune after Auto / Build Profile"}));
        QCheckBox* native = dlg->findChildren<QCheckBox*>()[1];
        QVERIFY(native->isChecked() && !native->isEnabled());               // the C++ UI always uses libnvcore
        QVERIFY(dlg->findChild<QLineEdit*>()->text().isEmpty());
        dlg->reject();
    }

    void preferencesAcceptWritesKeys() {   // OK writes prefs/<key> of the shared settings (Cancel keeps them)
        QDialog* dlg = win->preferences();
        QSpinBox* crf = nullptr;
        for (QSpinBox* s : dlg->findChildren<QSpinBox*>())
            if (!qobject_cast<QDoubleSpinBox*>(s) && s->maximum() == 51) crf = s;
        crf->setValue(30);
        dlg->reject();
        QCOMPARE(win->prefs["crf"].toInt(), 16);
        dlg = win->preferences();
        for (QSpinBox* s : dlg->findChildren<QSpinBox*>())
            if (!qobject_cast<QDoubleSpinBox*>(s) && s->maximum() == 51) s->setValue(22);
        for (QComboBox* c : dlg->findChildren<QComboBox*>())
            if (c->findText("aac") >= 0) c->setCurrentText("aac");
        dlg->accept();
        QCOMPARE(win->prefs["crf"].toInt(), 22);
        QCOMPARE(win->prefs["audio"].toString(), QString("aac"));
        QSettings s(tmp.filePath("settings.ini"), QSettings::IniFormat);
        QCOMPARE(s.value("prefs/crf").toInt(), 22);
        QCOMPARE(s.value("prefs/audio").toString(), QString("aac"));
        QStringList keys;
        s.beginGroup("prefs");
        keys = s.childKeys();
        s.endGroup();
        keys.sort();
        QCOMPARE(keys, (QStringList{"audio", "auto_fine_tune", "auto_profile", "crf", "depth", "jobs", "mem_gb", "native",
                                    "preview_area", "profile_mode"}));               // main_window.PREFS (no ffmpeg: empty)
        QCOMPARE(win->depth_used, std::string("8"));                             // the depth did not change: no reload
        dlg = win->preferences();
        for (QSpinBox* sp : dlg->findChildren<QSpinBox*>())
            if (!qobject_cast<QDoubleSpinBox*>(sp) && sp->maximum() == 51) sp->setValue(16);
        for (QComboBox* c : dlg->findChildren<QComboBox*>())
            if (c->findText("aac") >= 0) c->setCurrentText("copy");
        dlg->accept();
        QCOMPARE(win->prefs["crf"].toInt(), 16);
        QCOMPARE(win->prefs["mem_gb"].toDouble(), 0.25);   // OK stores the clamped box value
        win->prefs["mem_gb"] = 0.05;                         // back to the test's memory cap (155 frames)
        QSettings(tmp.filePath("settings.ini"), QSettings::IniFormat).setValue("prefs/mem_gb", 0.05);
    }

    void profileViewerDetails() {   // "R1 Profile Viewer… (Shift+Ctrl+I)", "Quality / Completeness: Profile Viewer shows the value"
        win->tabs->setCurrentIndex(0);
        QVERIFY(wait_until([&] { return idle() && win->quality_pct.has_value(); }));
        win->actions["p_viewer"]->trigger();
        QDialog* dlg = find_dialog("Profile Viewer");
        QVERIFY(dlg);
        const PyObj& base = win->state->profile.base;
        const PyObj& a = base.get("area");
        const int h = static_cast<int>(a[2].as_int()), w = static_cast<int>(a.size() == 4 ? a[3].as_int() : a[2].as_int());
        QStringList texts;
        for (QLabel* l : dlg->findChildren<QLabel*>()) texts << l->text();
        QVERIFY2(texts.contains(QString::fromUtf8("%1x%2 at (x %3, y %4) · frame %5").arg(w).arg(h).arg(a[1].as_int())
                                    .arg(a[0].as_int()).arg(base.get("ui_frame").as_int())), qPrintable(texts.join("|")));
        QVERIFY(texts.contains("Sample area:") && texts.contains("Match:") && texts.contains("Band ratios (Y, repro):"));
        const QString d = dlg->findChild<QTextEdit*>()->toPlainText();
        QVERIFY(d.startsWith("device: Unspecified device\nmode: auto"));
        QVERIFY(d.contains(QString("Overall Profile Quality: %1% (genuine measure, quality.py)").arg(*win->quality_pct)));
        QVERIFY(d.contains(QString("Completeness: %1% (genuine measure, quality.py)").arg(*win->completeness_pct)));
        QVERIFY(d.contains("profile Noise Level: all +0%, Y +0%, Cr +0%, Cb +0%"));
        bool graph = false;
        for (QWidget* w : dlg->findChildren<QWidget*>()) graph = graph || dynamic_cast<NoiseGraph*>(w) != nullptr;
        QVERIFY(graph);
        auto* mode = dlg->findChild<QComboBox*>();
        QCOMPARE(mode->currentText(), QString("auto"));
        mode->setCurrentText("spatial");                       // the build_profile_v5 mode, stored at once
        QCOMPARE(win->prefs["profile_mode"].toString(), QString("spatial"));
        QCOMPARE(QSettings(tmp.filePath("settings.ini"), QSettings::IniFormat).value("prefs/profile_mode").toString(),
                 QString("spatial"));
        mode->setCurrentText("auto");
        dlg->close();
        QCoreApplication::processEvents();
        win->profile_panel->icons->info->click();             // the panel's (i) opens it too
        dlg = find_dialog("Profile Viewer");
        QVERIFY(dlg);
        dlg->close();
    }

    void profileViewerWithoutProfile() {
        win->reset_profile();
        QDialog* dlg = win->profile_viewer();
        QStringList texts;
        for (QLabel* l : dlg->findChildren<QLabel*>()) texts << l->text();
        QVERIFY(texts.contains("(no profile)"));
        QCOMPARE(dlg->findChild<QTextEdit*>()->toPlainText(), QString("no profile"));
        dlg->close();
        win->state->undo->undo();
        QVERIFY(win->state->profile.has_profile());
    }

    void batchProfilerRuns() {   // "batch profiler": frame N // 3 of each video, <stem>.nvprof.json
        const QString d = tmp.filePath("batch");
        QDir().mkpath(d);
        const QStringList files{test_video()};
        QVERIFY(win->batch_profiler(&files, d, "auto") == nullptr);
        QVERIFY(wait_until([&] { return idle() && win->batch_result.has_value(); }));
        QCOMPARE(static_cast<int>(win->batch_result->size()), 1);
        const QString p = d + "/test.nvprof.json";
        QCOMPARE(win->batch_result->front(), p);
        QCOMPARE(win->msg->text(), "batch profiler: wrote 1 profiles to " + d);
        const ProfileState ps = ProfileState::from_json(PyObj::parse(text_of(p).toStdString()));
        QCOMPARE(ps.device, std::string("test.mp4"));
        const VideoInfo vi = probe(test_video().toStdString());
        const int p0 = std::max(std::min(vi.n / 3, vi.n - 3), 0);
        QCOMPARE(ps.base.get("ui_frame").as_int(), static_cast<long long>(p0));
        std::vector<Frame> fr;
        read_frames(test_video().toStdString(), vi.W, vi.H, p0, p0 + 3, vi.fps, [&](int, Frame&& f) {
            fr.push_back(std::move(f));
            return true;
        });
        PyObj exp = build_profile(fr, 0, vi.full_range, matrix_from_tags(vi.tags, vi.H), "auto", nullptr, nullptr);
        exp.set("ui_frame", PyObj::integer(p0));
        QVERIFY(ps.base == exp);
        QCOMPARE(text_of(p), QString::fromStdString(ps.to_json().dump(1)));
    }

    void batchProfilerDialog() {
        win->actions["batch"]->trigger();
        QDialog* dlg = find_dialog("Batch Profiler");
        QVERIFY(dlg);
        QVERIFY(dlg->findChild<QListWidget*>() != nullptr);
        QStringList b;
        for (QPushButton* x : dlg->findChildren<QPushButton*>()) b << x->text();
        QCOMPARE(b, (QStringList{QString::fromUtf8("Add videos…"), "Choose output folder and run"}));
        auto* m = dlg->findChild<QComboBox*>();
        QCOMPARE(m->count(), 3);
        QCOMPARE(m->itemText(2), QString("temporal"));
        dlg->reject();
    }

    void genericProfileDefaultOrReset() {   // Generic Profile: the Default Profile, else no profile (repro)
        const std::string orig = win->state->profile.to_json().dump(1);
        const int i0 = win->state->undo->index();
        win->assign_default_profile();
        win->reset_profile();
        win->generic_btn->click();
        QCOMPARE(win->msg->text(), QString("Generic Profile: Default Profile loaded (repro)"));
        QCOMPARE(win->state->profile.to_json().dump(1), orig);
        win->reset_default_profile();
        win->actions["p_generic"]->trigger();
        QVERIFY(!win->state->profile.has_profile());
        QCOMPARE(win->msg->text(), QString("Generic Profile: no Default Profile assigned, profile reset (repro)"));
        while (win->state->undo->index() > i0) win->state->undo->undo();
        QCOMPARE(win->state->profile.to_json().dump(1), orig);
    }

    void componentPanesEveryMode() {   // "component panes computed for every mode"
        win->tabs->setCurrentIndex(0);
        const std::string m0 = win->viewer->mode();
        for (const auto& [mode, _] : MODES) {
            win->viewer->set_mode(mode);
            QVERIFY(wait_until([&] { return win->viewer->ready(); }, 120));
            const auto ps = win->viewer->panes();
            QVERIFY(!ps.empty());
            for (const auto& [name, r] : ps) QVERIFY2(!win->viewer->pane_image(name, win->viewer->source()).isNull(), name.c_str());
            if (mode == "Normal") QCOMPARE(static_cast<int>(ps.size()), 1);
        }
        win->viewer->set_mode(m0);
    }

    void staleAftDiscardedAndButtons() {   // "stale AFT result discarded", "AFT buttons re-enabled"
        const int i0 = win->state->undo->index();
        win->auto_fine_tune();
        QVERIFY(!win->profile_panel->b_aft->isEnabled());
        win->reset_profile();
        QVERIFY(wait_until([&] { return idle(); }));
        QVERIFY(!win->state->profile.has_profile());
        QCOMPARE(win->msg->text(), QString("Auto Fine-Tune result discarded: the profile changed while it ran"));
        QVERIFY(win->profile_panel->b_aft->isEnabled() && win->actions["p_aft"]->isEnabled());
        while (win->state->undo->index() > i0) win->state->undo->undo();
        QVERIFY(win->state->profile.has_profile());
    }

    void aftCancellable() {   // "AFT cancellable": the native AFT stops at its next tile query
        const std::string before = win->state->profile.to_json().dump(1);
        QElapsedTimer t;
        t.start();
        win->auto_fine_tune();
        win->runner->cancel("aft");
        QVERIFY(wait_until([&] { return idle(); }, 30));
        QVERIFY(t.elapsed() < 5000);
        QVERIFY(!win->runner->busy("aft"));
        QCOMPARE(win->state->profile.to_json().dump(1), before);
        QVERIFY(win->profile_panel->b_aft->isEnabled());
    }

    void aftBinFlagsStored() {   // "Quality: AFT bin flags stored (3x9 bools each)"
        const PyObj& f = win->state->profile.base.get("aft_bin_flags");
        for (const char* k : {"marker", "clip", "gt3"}) {
            QCOMPARE(static_cast<int>(f.get(k).size()), 3);
            for (int c = 0; c < 3; ++c) {
                QCOMPARE(static_cast<int>(f.get(k)[static_cast<size_t>(c)].size()), 9);
                for (int b = 0; b < 9; ++b) QCOMPARE(f.get(k)[static_cast<size_t>(c)][static_cast<size_t>(b)].type(), PyObj::BOOL);
            }
        }
    }

    void qualityFreshResetAndNone() {   // "AFT raises the value", "fresh profile is 0-1%", "after Reset", "n/a without a profile"
        ProfileState fresh = win->state->profile;
        fresh.base.pop("aft_applied");
        fresh.base.pop("aft_bin_flags");
        fresh.base.pop("ui_pre_aft");
        const auto qa = profile_numbers(win->state->profile, win->frames, win->full_range, win->matrix);
        const auto qf = profile_numbers(fresh, win->frames, win->full_range, win->matrix);
        QVERIFY(qa && qf);
        QVERIFY(qa->first > qf->first);
        QVERIFY(qf->second <= 1);
        ProfileState reset = win->state->profile;
        reset.base.set("aft_reset", PyObj::boolean(true));
        reset.factors = unit_factors();
        const auto qr = profile_numbers(reset, win->frames, win->full_range, win->matrix);
        QVERIFY(qr && qr->second == 0 && qr->first < qa->first);
        win->reset_profile();
        QVERIFY(wait_until([&] { return idle(); }));
        QCOMPARE(win->profile_panel->quality->text(), QString("n/a"));
        QCOMPARE(win->profile_panel->completeness->text(), QString("n/a"));
        win->state->undo->undo();
        QVERIFY(wait_until([&] { return idle() && win->quality_pct.has_value(); }));
    }

    void profileJsonNonFiniteRejected() {   // "profile JSON with NaN / inf tune, factors or curve rejected"
        const std::string good = win->state->profile.to_json().dump(1);
        int rejected = 0;
        for (int mut = 0; mut < 3; ++mut) {
            PyObj e = PyObj::parse(good);
            if (mut == 0) {
                PyObj t = PyObj::dict();
                t.set("all", PyObj::real(std::nan("")));
                e.set("tune", t);
            } else if (mut == 1) {
                e.at("factors")[0][3] = PyObj::real(INFINITY);
            } else {
                e.at("base").at("curve").at("Y")[0] = PyObj::real(std::nan(""));
            }
            const QString bad = tmp.filePath(QString("bad_%1.json").arg(mut));
            std::ofstream(bad.toStdString()) << e.dump();
            win->load_profile(bad);
            rejected += win->state->profile.to_json().dump(1) == good && win->msg->text().startsWith("cannot load profile");
        }
        QCOMPARE(rejected, 3);
    }

    void saveFramePng() {   // "preview modes + save frame"
        const QString p = tmp.filePath("frame.png");
        win->save_frame(p);
        QVERIFY(QFileInfo(p).size() > 0);
        QCOMPARE(win->msg->text(), "saved " + p);
        QVERIFY(!QImage(p).isNull());
    }

    void selectionPercentVsMain() {   // "R3 selection Noise Level (%+d%) vs Main Sample"
        win->tabs->setCurrentIndex(0);
        const PyObj& base = win->state->profile.base;
        const PyObj& a = base.get("area");
        const int y = static_cast<int>(a[0].as_int()), x = static_cast<int>(a[1].as_int()), h = static_cast<int>(a[2].as_int()),
                  w = static_cast<int>(a.size() == 4 ? a[3].as_int() : a[2].as_int());
        win->timeline->set_frame(static_cast<int>(base.get("ui_frame").as_int()), true);
        const auto mainv = win->nl(win->timeline->frame(), Rect{y, x, h, w});
        QVERIFY(mainv);
        win->on_area(y, x, h, w);
        char b[64];
        std::snprintf(b, sizeof(b), "Noise Level: %.1f (+0%%)", *mainv);
        QCOMPARE(win->viewer->sel_label(), QString(b));
        const int H = win->frames[0].H, W = win->frames[0].W;
        win->on_area(H / 2, W / 4, 64, 96);
        const auto selv = win->nl(win->timeline->frame(), Rect{H / 2, W / 4, 64, 96});
        const int pct = static_cast<int>((*selv / *mainv - 1.0) * 100.0);
        std::snprintf(b, sizeof(b), "Noise Level: %.1f (%s%d%%)", *selv, pct >= 0 ? "+" : "-", std::abs(pct));
        QCOMPARE(win->viewer->sel_label(), QString(b));
        win->reset_profile();
        std::snprintf(b, sizeof(b), "Noise Level: %.1f", *selv);
        QCOMPARE(win->viewer->sel_label(), QString(b));
        win->state->undo->undo();
        win->sel_area.reset();
        win->sync_selection();
    }

    void pcheckRecomputesOnNoiseLevel() {   // "R1 Noise Level slider recomputes the Assist Image; undo shows the cached one"
        win->tabs->setCurrentIndex(0);
        win->set_profile_check(true);
        QVERIFY(wait_until([&] { return win->viewer->showing_pcheck() && idle() && win->timers_idle(); }));
        const std::vector<float> a = win->viewer->image("pcheck").d;
        win->state->new_edit();
        win->state->set_tune("all", 1.5);
        QVERIFY(wait_until([&] { return idle() && win->timers_idle() && win->viewer->image("pcheck").d != a; }));
        win->state->undo->undo();
        QVERIFY(wait_until([&] { return idle() && win->timers_idle(); }));
        QVERIFY(win->viewer->image("pcheck").d == a);
        win->set_profile_check(false);
    }

    void everyControlChangesBackend() {   // "every implemented control changes the backend parameters"
        const std::string base_P = backend_params(default_params()).dump(-1, true);
        QStringList missing;
        for (const auto& [k, sp] : param_specs()) {
            if (!sp.implemented) continue;
            PyObj ui = default_params();
            if (k == "mix") ui.set("mix_enable", PyObj::boolean(true));
            if (k == "mix_enable") ui.set("mix", PyObj::real(25.0));
            const PyObj& d = ui.get(k);
            if (sp.kind == "bool")
                ui.set(k, PyObj::boolean(!d.truthy()));
            else if (sp.kind == "choice")
                for (const auto& c : sp.choices) {
                    if (c != d.as_str()) {
                        ui.set(k, PyObj::str(c));
                        break;
                    }
                }
            else if (sp.kind == "int")
                ui.set(k, PyObj::integer(static_cast<long long>(d.as_double() != sp.lo ? sp.lo : sp.hi)));
            else
                ui.set(k, PyObj::real(d.as_double() != sp.lo ? sp.lo : sp.hi));
            if (backend_params(ui).dump(-1, true) == base_P) missing << QString::fromStdString(k);
        }
        QVERIFY2(missing.isEmpty(), qPrintable(missing.join(",")));
    }

    void naControlsDisabled() {   // "unimplemented controls are disabled", "non-implemented controls show 'n/a'"
        QStringList bad;
        for (const auto& [k, sp] : param_specs()) {
            if (sp.implemented) continue;
            auto it = win->filter_panel->controls.find(k);
            if (it == win->filter_panel->controls.end()) continue;
            QWidget* w = it->second;
            QString t;
            if (auto* r = qobject_cast<Row*>(w)) t = r->val->text();
            else if (auto* e = qobject_cast<EnableCheck*>(w)) t = e->na_label ? e->na_label->text() : e->text();
            else if (auto* c = qobject_cast<ChoiceRow*>(w)) t = c->label->text();
            if (w->isEnabled() || !t.contains("n/a")) bad << QString::fromStdString(k);
        }
        QVERIFY2(bad.isEmpty(), qPrintable(bad.join(",")));
        QVERIFY(win->actions["p_match"]->text().contains("n/a") && !win->actions["p_match"]->isEnabled());
        QVERIFY(win->profile_panel->b_manual->text().contains("n/a"));
    }

    void trackRangesAndDefaults() {   // "Noise Level track -100..+150, Mix off shows 75%, Radius '2', no orange frames on Prepare"
        Row* nr = win->filter_panel->row("noise_Y");
        Row* ov = win->filter_panel->row("adv_noise_level");
        QCOMPARE(nr->slider->minimum(), -100);
        QCOMPARE(nr->slider->maximum(), 150);
        QCOMPARE(ov->slider->minimum(), -100);
        QCOMPARE(ov->slider->maximum(), 150);
        QCOMPARE(ov->spin->maximum(), 300.0);
        QCOMPARE(spec("noise_Y").hi, 250.0);
        QCOMPARE(win->filter_panel->row("mix")->val->text(), QString("75%"));
        QVERIFY(!win->state->params.get("mix_enable").truthy());
        QCOMPARE(win->filter_panel->row("t_radius")->val->text(), QString("2"));
        win->tabs->setCurrentIndex(0);
        QCOMPARE(win->timeline->thumbs->radius, 0);
        win->tabs->setCurrentIndex(1);
        QCOMPARE(win->timeline->thumbs->radius, 2);
        win->tabs->setCurrentIndex(0);
    }

    void panelNoScrollBarAndOrder() {   // "R2 panel: no scroll bar at 1800x1125 (Noise Level + Frequencies open), ... collapsed defaults"
        const QSize s0 = win->size();
        win->resize(1800, 1125);
        ProfilePanel* pp = win->profile_panel;
        pp->tune_header->set_expanded(true);
        pp->freq_header->set_expanded(true);
        for (int i = 0; i < 5; ++i) QCoreApplication::processEvents();
        QScrollBar* sb = pp->scroll->verticalScrollBar();
        QVERIFY(!sb->isVisible() && sb->maximum() == 0);
        QCOMPARE(pp->b_reset->toolTip(), QString("Reset all fine-tuning"));
        QVERIFY(!pp->b_manual->isEnabled());
        ProfilePanel fresh;
        QVERIFY(fresh.tune_header->body->isHidden() && fresh.freq_header->body->isHidden());
        pp->tune_header->set_expanded(false);
        pp->freq_header->set_expanded(false);
        win->resize(s0);
        QCoreApplication::processEvents();
    }

    void depthConversionsRoundTrip() {   // nv_depth through libnvcore: integer formats round-trip exactly, NaN / inf restored
        std::mt19937 rng(7);
        for (const char* name : {"yuv420p10le", "yuv422p12le", "yuv444p16le", "gbrp10le", "gbrp16le"}) {
            const PixFmt f = pixfmt(name);
            const int W = 37, H = 21;
            std::vector<uint8_t> raw(static_cast<size_t>(f.frame_bytes(W, H)));
            for (size_t i = 0; i + 1 < raw.size(); i += 2) {
                const uint16_t u = static_cast<uint16_t>(rng() % (1u << f.bits));
                std::memcpy(raw.data() + i, &u, 2);
            }
            const Planes p = to_internal(raw.data(), f, W, H, "bt709", nullptr);
            if (f.family == "yuv") QVERIFY2(from_internal(p, f, "bt709", nullptr) == raw, name);    // rgb: matrix rounding
            QCOMPARE(p.Wc, f.chroma == "420" ? (W + 1) / 2 : W);
        }
        const PixFmt f = pixfmt("gbrpf32le");
        const int W = 8, H = 4, n = W * H;
        std::vector<float> v(3 * static_cast<size_t>(n), 0.5f);
        v[3] = std::nanf("");
        v[n + 5] = INFINITY;
        v[2 * n + 7] = -INFINITY;
        std::vector<uint8_t> raw(v.size() * 4);
        std::memcpy(raw.data(), v.data(), raw.size());
        RestoreMap rm;
        const Planes p = to_internal(raw.data(), f, W, H, "bt709", &rm);
        QCOMPARE(static_cast<int>(rm.size()), 3);
        for (const float x : p.Y) QVERIFY(std::isfinite(x));
        const auto back = from_internal(p, f, "bt709", &rm);
        float b3, b5, b7;
        std::memcpy(&b3, back.data() + 3 * 4, 4);
        std::memcpy(&b5, back.data() + static_cast<size_t>(n + 5) * 4, 4);
        std::memcpy(&b7, back.data() + static_cast<size_t>(2 * n + 7) * 4, 4);
        QVERIFY(std::isnan(b3) && b5 == INFINITY && b7 == -INFINITY);
    }

    void ofxWindowSetup() {   // the OFX host mode window: labels, Performance, the standalone-only controls off
        std::vector<Frame> fr(win->frames.begin(), win->frames.begin() + 5);
        auto* ow = new MainWindow();
        ow->set_ofx_frames(std::move(fr), 640, 360, 30.0, 16);
        QCOMPARE(ow->src, QString("<OFX host>"));
        QCOMPARE(ow->video_lbl->text(), QString::fromUtf8("OFX host: 640x360 · 5 frames · 30.0 fps"));
        QCOMPARE(ow->depth_lbl->text(), QString::fromUtf8("source OFX host (16-bit 4:2:0) · processing 16-bit"));
        QVERIFY(ow->filter_panel->perf->text().contains("Frame Bitdepth:&nbsp;&nbsp;</td><td>16-bit (OFX host)"));
        QCOMPARE(ow->timeline->frame(), 1);
        for (const char* k : {"open", "export"}) QVERIFY(!ow->actions[k]->isEnabled());
        QVERIFY(!ow->load_btn->isEnabled() && !ow->generic_btn->isEnabled());
        QCOMPARE(ow->b_apply->toolTip(), QString("Apply: process the current frame and emit to the host"));
        QCOMPARE(ow->msg->text(), QString("OFX host: Auto Profile (Prepare Noise Profile), adjust, then Apply to return the frame to the host"));
        QWidget* box = ow->image_info();
        QVERIFY(qobject_cast<QMessageBox*>(box)->text().startsWith("<OFX host>\n640x360, source unknown (8-bit 4:2:0), read as yuv420p"));
        box->close();
        delete ow;
    }

    void ofxArgsAndNoFrames() {   // main.py _ofx_main: usage / no frames on stdin -> exit 1, nothing on stdout
        const QString usage = "usage: --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]";
        for (const QStringList& a : {QStringList{"640"}, QStringList{"640", "360", "--depth", "12"}, QStringList{"0", "360"},
                                     QStringList{"640", "360", "x"}}) {
            const OfxRun r = ofx_run(a, QByteArray(), "close");
            QCOMPARE(r.code, 1);
            QVERIFY(r.out.isEmpty());
            QVERIFY2(QString(r.err).contains(usage), qPrintable(a.join(" ")));
        }
        const OfxRun r = ofx_run({"64", "36"}, QByteArray(10, '\0'), "close");
        QCOMPARE(r.code, 1);
        QVERIFY(QString(r.err).contains("no frames received on stdin"));
        // W / H beyond int, a frame size beyond 64 bits, an integer beyond 64 bits (N3c verification L-1): never a
        // complete frame -> "no frames received on stdin", exit 1 (as Python), within the timeout
        const QByteArray two = ofx_bytes(100, 2, 8);
        for (const QStringList& a : {QStringList{"4294967296", "120"}, QStringList{"2147483648", "1", "24", "2"},
                                     QStringList{"4294967496", "120", "24", "2"}, QStringList{"99999999999999999999", "120"},
                                     QStringList{"3037000500", "3037000500", "24", "0", "--depth", "32"}}) {
            const OfxRun q = ofx_run(a, a[0] == "4294967296" ? QByteArray() : two, "close", 30000);
            QVERIFY2(q.code == 1 && q.out.isEmpty() && QString(q.err).contains("no frames received on stdin"), qPrintable(a.join(" ")));
        }
    }

    void ofxRoundTrip() {   // close = the current frame (Auto Profile of it) at --depth 8 / 16 / 32, NaN / -inf written back
        const PyObj P = backend_params(default_params());
        for (int d : {8, 16, 32}) {
            const QByteArray in = ofx_bytes(100, 5, d);
            const OfxRun r = ofx_run({"640", "360", "30", "0", "--depth", QString::number(d)}, in, "close");
            QCOMPARE(r.code, 0);
            QCOMPARE(r.out.size(), static_cast<qsizetype>(ofx_frame_size(640, 360, d)));
            QVERIFY2(r.out == ofx_expected(in, 640, 360, d, 1, P), qPrintable(QString("depth %1").arg(d)));
            if (d == 32) {
                float y1000;
                std::memcpy(&y1000, r.out.constData() + 4 * 1000, 4);
                QVERIFY(std::isnan(y1000));
            }
        }
    }

    void ofxApplySameAndCancel() {   // Apply writes, an unchanged Apply / close writes nothing more; Cancel writes nothing
        const QByteArray in = ofx_bytes(100, 5, 8);
        const qsizetype fs = static_cast<qsizetype>(ofx_frame_size(640, 360, 8));
        OfxRun r = ofx_run({"640", "360"}, in, "apply;apply;close");
        QCOMPARE(r.out.size(), fs);
        QVERIFY(QString(r.err).contains("MSG: Apply: frame 1 and the settings are unchanged since the last Apply (not written again)"));
        r = ofx_run({"640", "360"}, in, "apply;set amt_Y 40;apply;cancel");
        QCOMPARE(r.out.size(), 2 * fs);
        PyObj ui = default_params();
        ui.set("amt_Y", PyObj::real(40.0));
        QVERIFY(r.out.mid(fs) == ofx_expected(in, 640, 360, 8, 1, backend_params(ui)));
        r = ofx_run({"640", "360"}, in, "cancel");
        QCOMPARE(r.code, 0);
        QVERIFY(r.out.isEmpty());
        r = ofx_run({"640", "360", "24", "2"}, in, "close");     // MAX_FRAMES 2: frame min(2 // 3, 0) = 0
        QVERIFY(r.out == ofx_expected(in.left(2 * fs), 640, 360, 8, 0, backend_params(default_params())));
    }

    void resetUsesDefaultPreset() {   // "reset uses assigned default preset"
        win->state->new_edit();
        win->state->set_param("noise_Cr", PyObj::real(150.0));
        win->assign_default_preset();
        win->reset_filter();
        QCOMPARE(win->state->params.get("noise_Cr").as_double(), 150.0);
        win->reset_default_preset();
        win->reset_filter();
        QCOMPARE(win->state->params.get("noise_Cr").as_double(), 100.0);
    }

    void wheelUndoSteps() {   // "wheel: drag = 1, drag + wheel = 2, two wheel gestures = 2 undo steps"
        win->tabs->setCurrentIndex(1);
        QSlider* sl = win->filter_panel->row("amt_Y")->slider;
        QCOMPARE(win->state->params.get("amt_Y").as_double(), 70.0);
        auto wheel = [&] {
            const QPointF c(sl->width() / 2.0, sl->height() / 2.0);
            QWheelEvent ev(c, sl->mapToGlobal(c), QPoint(0, 0), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QApplication::sendEvent(sl, &ev);
            QCoreApplication::processEvents();
        };
        const int i0 = win->state->undo->index();
        sl->setSliderDown(true);
        for (int dv : {-5, -10, -20}) sl->setSliderPosition(70 + dv);
        sl->setSliderDown(false);
        QCOMPARE(win->state->undo->index(), i0 + 1);
        QCOMPARE(win->state->params.get("amt_Y").as_double(), 50.0);
        wheel();
        QCOMPARE(win->state->undo->index(), i0 + 2);
        QVERIFY(win->state->params.get("amt_Y").as_double() != 50.0);
        QTest::qWait(300);
        wheel();
        QCOMPARE(win->state->undo->index(), i0 + 3);
        win->state->undo->undo();
        win->state->undo->undo();
        QCOMPARE(win->state->params.get("amt_Y").as_double(), 50.0);
        win->state->undo->undo();
        QCOMPARE(win->state->params.get("amt_Y").as_double(), 70.0);
    }

    void sidePanelsFitAtMinWidth() {   // "side panels fit at the minimum window width (all sections expanded, both modes)"
        const QSize s0 = win->size();
        std::vector<std::pair<Header*, bool>> hs;
        for (QWidget* w : win->findChildren<QWidget*>())
            if (auto* h = dynamic_cast<Header*>(w)) {
                hs.emplace_back(h, !h->body->isHidden());
                h->set_expanded(true);
            }
        win->resize(win->minimumSizeHint().width(), s0.height());
        QStringList over;
        for (bool adv : {true, false}) {
            win->set_advanced(adv);
            for (int page : {0, 1}) {
                win->tabs->setCurrentIndex(page);
                std::vector<QScrollArea*> scs;
                if (page == 1) {
                    for (int i = 0; i < win->filter_panel->tabs->count(); ++i)
                        scs.push_back(qobject_cast<QScrollArea*>(win->filter_panel->tabs->widget(i)));
                } else {
                    for (QScrollArea* a : win->profile_panel->findChildren<QScrollArea*>()) scs.push_back(a);
                }
                for (size_t i = 0; i < scs.size(); ++i) {
                    QVERIFY(scs[i]);
                    if (page == 1) win->filter_panel->tabs->setCurrentIndex(static_cast<int>(i));
                    for (int k = 0; k < 3; ++k) QCoreApplication::processEvents();
                    if (scs[i]->widget()->minimumSizeHint().width() > scs[i]->viewport()->width())
                        over << QString("%1/%2/%3: %4 > %5").arg(adv).arg(page).arg(i).arg(scs[i]->widget()->minimumSizeHint().width())
                                    .arg(scs[i]->viewport()->width());
                }
            }
        }
        win->set_advanced(true);
        for (auto& [h, on] : hs) h->set_expanded(on);
        win->filter_panel->tabs->setCurrentIndex(0);
        win->tabs->setCurrentIndex(0);
        win->resize(s0);
        QCoreApplication::processEvents();
        QVERIFY2(over.isEmpty(), qPrintable(over.join("; ")));
    }

    // ---------------- Bit depth (last: the window reloads its video)
    void bitDepth10Reload() {   // "10-bit: Bit depth 10 reloads as yuv420p10le float32 planes (0..255 scale)"
        v10 = tmp.filePath("src10.mkv");
        QProcess p8;
        p8.start(QString::fromStdString(tool_path("ffmpeg")), QStringList{"-v", "error", "-i", test_video(), "-frames:v", "12", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"});
        QVERIFY(p8.waitForFinished(60000));
        const QByteArray raw8 = p8.readAllStandardOutput();
        std::mt19937 rng(10);
        a10.resize(raw8.size() * 2);
        for (qsizetype i = 0; i < raw8.size(); ++i) {
            const uint16_t u = static_cast<uint16_t>(static_cast<uint8_t>(raw8[i]) * 4 + rng() % 4);
            std::memcpy(a10.data() + 2 * i, &u, 2);
        }
        QProcess pe;
        pe.start(QString::fromStdString(tool_path("ffmpeg")), QStringList{"-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv420p10le", "-s", "640x360", "-r", "30",
                                       "-colorspace", "bt709", "-color_range", "tv", "-i", "-", "-c:v", "ffv1", "-pix_fmt",
                                       "yuv420p10le", "-colorspace", "bt709", "-color_range", "tv", v10});
        QVERIFY(pe.waitForStarted());
        pe.write(a10);
        pe.closeWriteChannel();
        QVERIFY(pe.waitForFinished(60000) && pe.exitCode() == 0);
        win->tabs->setCurrentIndex(0);
        win->open_video(v10);
        QVERIFY(wait_until([&] { return win->src == v10 && aft_done(); }));
        QCOMPARE(win->fmt->name, std::string("yuv420p"));                  // Bit depth 8: the legacy path
        QDialog* dlg = win->preferences();
        for (QComboBox* c : dlg->findChildren<QComboBox*>())
            if (c->findData("10") >= 0) c->setCurrentIndex(c->findData("10"));
        dlg->accept();
        QVERIFY(wait_until([&] { return win->fmt && win->fmt->bits == 10 && aft_done(); }));
        QCOMPARE(win->fmt->name, std::string("yuv420p10le"));
        QCOMPARE(win->depth_used, std::string("10"));
        const Frame& f0 = win->frames[0];
        QVERIFY(f0.is_float() && f0.Y.empty());
        QVERIFY(f0.Hc != f0.H);
        float mx = 0;
        for (float x : f0.fY) mx = std::max(mx, x);
        QVERIFY(mx <= 256.0f);
    }

    void bitDepth10ExactValues() {   // "10-bit: frame 0 = the 10-bit values / 4 exactly (low bits kept)"
        const Frame& f0 = win->frames[0];
        const size_t n0 = f0.fY.size() + f0.fCb.size() + f0.fCr.size();
        int low = 0;
        size_t k = 0;
        for (const auto* pl : {&f0.fY, &f0.fCb, &f0.fCr})
            for (float x : *pl) {
                uint16_t u;
                std::memcpy(&u, a10.constData() + 2 * k++, 2);
                QVERIFY(x == static_cast<float>(u) / 4.0f);
                low += u % 4 != 0;
            }
        QVERIFY(low > static_cast<int>(n0 / 2));
    }

    void bitDepth10StatusBar() {   // "10-bit: status bar shows source pix_fmt and processing depth"
        const QString lbl = win->depth_lbl->text();
        QVERIFY2(lbl.contains("yuv420p10le (10-bit 4:2:0)") && lbl.contains("processing 10-bit"), qPrintable(lbl));
        QVERIFY(win->viewer_bar->size->text().contains("10-bit"));
        QVERIFY(win->filter_panel->perf->text().contains("Frame Bitdepth:&nbsp;&nbsp;</td><td>10-bit (yuv420p10le)"));
        QWidget* box = win->image_info();
        QVERIFY(qobject_cast<QMessageBox*>(box)->text().contains(QString::fromUtf8("read as yuv420p10le · 10-bit 4:2:0")));
        box->close();
    }

    void bitDepth10Preview() {   // "10-bit: preview filtered (8-bit display)"
        win->tabs->setCurrentIndex(1);
        win->invalidate();
        win->schedule();
        QVERIFY(wait_until([&] { return idle() && win->timers_idle() && win->viewer->showing_filtered(); }));
    }

    void bitDepth10ExportFilter() {   // "10-bit: export dialog defaults"
        QCOMPARE(win->export_filter(), std::make_pair(QString("MP4 (*.mp4);;QuickTime (*.mov);;Matroska FFV1 (*.mkv)"), QString(".mp4")));
    }

    void bitDepth10ExportEqualsProcessFrame() {   // "10-bit: export (FFV1 yuv420p10le) == backend.process_frame bit for bit"
        const QString dst = tmp.filePath("export10.mkv");
        const int lo = 4, n = 3;
        win->process_video(dst, lo, n);
        QVERIFY(wait_until([&] { return win->export_ == nullptr; }, 1800) && win->export_ok);
        const VideoInfo vi = probe(dst.toStdString());
        QCOMPARE(vi.pix_fmt, std::string("yuv420p10le"));
        const PixFmt f = *win->fmt;
        std::vector<Planes> got;
        read_frames_fmt(dst.toStdString(), 640, 360, 0, -1, "", f, "bt709", "", [&](int, Planes&& p) {
            got.push_back(std::move(p));
            return true;
        });
        QCOMPARE(static_cast<int>(got.size()), n);
        const PyObj P = win->state->backend(), prof = *win->state->effective();
        for (int k = 0; k < n; ++k) {
            auto [idx, wv] = win->engine->window(lo + k, static_cast<int>(P.get("radius").as_int()), std::nullopt);
            const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), lo + k) - idx.begin());
            Planes tmp, rv;
            temporal(wv, ci, prof, P, nullptr, &tmp, &rv);
            const Planes o = finish(wv[static_cast<size_t>(ci)], tmp, rv, prof, P);
            QVERIFY2(from_internal(o, f, win->matrix, nullptr) == from_internal(got[static_cast<size_t>(k)], f, win->matrix, nullptr),
                     qPrintable(QString("frame %1").arg(lo + k)));
        }
    }

    void bitDepthMemoryCapAndFloat() {   // the memory cap counts the float32 bytes; 32 = gbrpf32le (full-range 4:4:4)
        win->tabs->setCurrentIndex(0);
        win->open_video(test_video());
        QVERIFY(wait_until([&] { return win->src == test_video() && aft_done(); }));
        const double mem = win->prefs["mem_gb"].toDouble();    // 0.25: the Bit depth dialog's OK stored the clamped value
        const long long cap = static_cast<long long>(mem * 1073741824.0) / pixfmt("yuv420p10le").internal_bytes(640, 360);
        QCOMPARE(static_cast<long long>(win->frames.size()), cap);
        QVERIFY(win->video_lbl->text().endsWith(QString("/%1 frames (memory cap)").arg(901)));
        QDialog* dlg = win->preferences();
        for (QComboBox* c : dlg->findChildren<QComboBox*>())
            if (c->findData("32") >= 0) c->setCurrentIndex(c->findData("32"));
        dlg->accept();
        QVERIFY(wait_until([&] { return win->fmt && win->fmt->bits == 32 && aft_done(); }));
        QCOMPARE(win->fmt->name, std::string("gbrpf32le"));
        QVERIFY(win->full_range);
        QCOMPARE(win->frames[0].Hc, win->frames[0].H);
        QCOMPARE(static_cast<long long>(win->frames.size()),
                 static_cast<long long>(mem * 1073741824.0) / pixfmt("gbrpf32le").internal_bytes(640, 360));
        QVERIFY(win->depth_lbl->text().endsWith("processing 32-bit float RGB"));
        QCOMPARE(win->export_filter(), std::make_pair(QString("Matroska FFV1 (*.mkv);;EXR sequence (*.exr)"), QString(".mkv")));
        QWidget* box = win->image_info();
        QVERIFY(qobject_cast<QMessageBox*>(box)->text().contains("(processed as full-range YCbCr)"));
        box->close();
        dlg = win->preferences();
        for (QComboBox* c : dlg->findChildren<QComboBox*>())
            if (c->findData("8") >= 0) c->setCurrentIndex(c->findData("8"));
        dlg->accept();
        QVERIFY(wait_until([&] { return win->fmt && win->fmt->legacy() && aft_done(); }));
        QCOMPARE(static_cast<long long>(win->frames.size()),
                 std::min(901LL, static_cast<long long>(mem * 1073741824.0) / pixfmt("yuv420p").internal_bytes(640, 360)));
    }

    // ---------------- G1, the R/G/B equalizer (sandbox/omo/wp_qtui_fix/DESIGN_G1.md, decisions U1-U10) --------
    void g1FreshAndAft() {   // "G1 fresh profile ..." / "G1 run_aft ..." / "G1 build-time colour weights" / "G1 after AFT"
        const auto& fr = win->frames;
        const int f = std::min(4, static_cast<int>(fr.size()) - 3);
        const PyObj fresh = build_profile(fr, f, win->full_range, win->matrix, "auto", nullptr, CancelFn());
        ProfileState ps0;
        ps0.base = fresh;
        QCOMPARE(ps0.channels, std::string("RGB"));
        QVERIFY(ps0.factors == unit_factors());
        QCOMPARE(fresh.get("ui_rgb_weights").size(), size_t(3));
        QVERIFY(ps0.effective() == fresh);
        QCOMPARE(ps0.to_json().get("format").as_str(), std::string("nv_repro_profile_rgb"));
        const PyObj aft = run_aft(fresh, fr[static_cast<size_t>(f)], win->full_range, win->matrix, CancelFn());
        QVERIFY(fresh.get("ui_rgb_weights") == aft.get("aft_color_weights"));
        const auto& k = aft.keys();
        QCOMPARE(k[k.size() - 2], std::string("aft_levels"));
        QCOMPARE(k[k.size() - 1], std::string("ui_pre_aft"));
        // the R/G/B combination of the AFT's levels on the curve before AFT is the AFT's curve (the probe path)
        const int n = static_cast<int>(aft.get("curve_centers").size());
        std::vector<double> cx = aft.get("curve_centers").as_doubles(), cv, cw, lv, out(3 * static_cast<size_t>(n));
        for (const char* c : {"Y", "Cb", "Cr"})
            for (double v : aft.get("ui_pre_aft").get("curve").get(c).as_doubles()) cv.push_back(v);
        for (size_t c = 0; c < 3; ++c)
            for (size_t j = 0; j < 3; ++j) cw.push_back(aft.get("aft_color_weights")[c][j].as_double());
        for (size_t j = 0; j < 3; ++j)
            for (size_t b = 0; b < 9; ++b) lv.push_back(aft.get("aft_levels")[j][b].as_double());
        QCOMPARE(nvc_rgb_combine(cx.data(), n, cv.data(), cw.data(), lv.data(), nullptr, out.data()), 0);
        int ci = 0;
        for (const char* c : {"Y", "Cb", "Cr"}) {
            const auto got = aft.get("curve").get(c).as_doubles();
            for (int i = 0; i < n; ++i) QCOMPARE(std::memcmp(&got[static_cast<size_t>(i)], &out[static_cast<size_t>(ci * n + i)], 8), 0);
            ++ci;
        }
        const ProfileState ps1 = after_aft(ps0, aft);
        QVERIFY(ps1.factors == *rgb_reference(aft));
        QVERIFY(!(ps1.factors == unit_factors()));
        QVERIFY(ps1.effective() == aft);
        g1_aft_ = aft;
        win->state->set_profile(ps1, "G1 test");
        CurveEditor* ed = win->profile_panel->curve;
        QCOMPARE(ed->channels, std::string("RGB"));
        QCOMPARE(ed->target_buttons.size(), size_t(4));
        QVERIFY(ed->target_buttons.count("R") && ed->target_buttons.count("G") && ed->target_buttons.count("B"));
        QVERIFY(ed->target_buttons.at("R")->toolTip().endsWith("for the Red channel alone"));
        QVERIFY(ed->factors == ps1.factors);
    }

    void g1EditsResetComplete() {   // "G1 R bin 4 +20%" / "G1 All +20%" / "G1 Reset" / "G1 Auto Complete"
        const PyObj& aft = g1_aft_;
        QVERIFY(aft.is_dict());
        CurveEditor* ed = win->profile_panel->curve;
        Factors L = win->state->profile.factors;
        L[0][4] = 1.2;
        win->state->new_edit();
        const int undo0 = win->state->undo->index();
        emit ed->edited(0, L[0], 4);
        const PyObj e = *win->state->effective();
        QCOMPARE(e.get("ui_channels").as_str(), std::string("RGB"));
        const auto xs = aft.get("curve_centers").as_doubles();
        for (const char* c : {"Y", "Cb", "Cr"}) {
            const auto a = e.get("curve").get(c).as_doubles(), b = aft.get("curve").get(c).as_doubles();
            for (size_t i = 0; i < xs.size(); ++i) QCOMPARE(a[i] == b[i], std::fabs(xs[i] - 128.0) >= 32.0);
        }
        win->state->undo->undo();
        QCOMPARE(win->state->undo->index(), undo0);
        QVERIFY(*win->state->effective() == win->state->profile.base);
        // All channels, every bin +20%: the curve before AFT x sqrt(1.2)
        Factors U;
        for (auto& r : U) r.fill(1.2);
        const PyObj e2 = rgb_effective_profile(aft, U, PyObj::dict());
        for (const char* c : {"Y", "Cb", "Cr"}) {
            const auto a = e2.get("curve").get(c).as_doubles(), p = aft.get("ui_pre_aft").get("curve").get(c).as_doubles();
            for (size_t i = 0; i < a.size(); ++i) QVERIFY(std::fabs(a[i] / (p[i] * std::sqrt(1.2)) - 1.0) < 1e-6);
        }
        // Reset: the curve and sigma before AFT exactly, Completeness 0%
        const ProfileState r = reset_fine_tune(win->state->profile);
        const PyObj er = r.effective();
        QVERIFY(er.get("curve") == aft.get("ui_pre_aft").get("curve"));
        QVERIFY(er.get("sigma") == aft.get("ui_pre_aft").get("sigma"));
        const auto q = profile_numbers(r, win->frames, win->full_range, win->matrix);
        QVERIFY(q && q->second == 0);
        // Auto Complete keeps the bins the AFT measured
        const Factors ac = auto_complete(win->state->profile);
        const PyObj& mk = aft.get("aft_bin_flags").get("marker");
        for (int c = 0; c < 3; ++c)
            for (int b = 0; b < 9; ++b)
                if (!mk[static_cast<size_t>(c)][static_cast<size_t>(b)].truthy()) QCOMPARE(ac[c][b], win->state->profile.factors[c][b]);
    }

    void g1QualityAndFiles() {   // "G1 Quality ..." / "G1 new format ..." / "G1 older Y / Cb / Cr files ..."
        const PyObj& aft = g1_aft_;
        const ProfileState ps1 = win->state->profile;
        ProfileState leg = ps1;
        leg.channels = "YCbCr";
        leg.factors = unit_factors();
        const auto q1 = profile_numbers(ps1, win->frames, win->full_range, win->matrix);
        QVERIFY(q1 && q1 == profile_numbers(leg, win->frames, win->full_range, win->matrix));
        ProfileState pt = ps1;
        pt.tune.set("all", PyObj::real(1.1));
        QVERIFY(profile_numbers(pt, win->frames, win->full_range, win->matrix) == q1);
        ProfileState pe = ps1;
        pe.factors[0][4] = 1.2;
        pe.pinned[0].insert(4);
        pe.tune.set("all", PyObj::real(1.1));
        pe.tune.set("Cr", PyObj::real(0.8));
        const PyObj d = pe.to_json();
        QCOMPARE(d.get("format").as_str(), std::string("nv_repro_profile_rgb"));
        QCOMPARE(d.get("channels").as_str(), std::string("RGB"));
        QCOMPARE(d.get("version").as_int(), int64_t(2));
        const ProfileState back = ProfileState::from_json(PyObj::parse(d.dump(1)));
        QCOMPARE(back.channels, std::string("RGB"));
        QVERIFY(back.factors == pe.factors && back.pinned == pe.pinned);
        QCOMPARE(back.effective().dump(1), pe.effective().dump(1));
        for (int mut = 0; mut < 2; ++mut) {
            PyObj x = d;
            if (mut == 0) x.pop("channels");
            else x.at("base").pop("ui_pre_aft");
            bool rejected = false;
            try {
                ProfileState::from_json(x);
            } catch (const std::runtime_error&) {
                rejected = true;
            }
            QVERIFY(rejected);
        }
        // older Y / Cb / Cr files: the effective profile the code before G1 wrote
        const QString dir = QString(NV_SOURCE_ROOT) + "/qtui/tests/fixtures";
        auto text = [](const QString& p) { std::ifstream f(p.toStdString(), std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); };
        for (const char* name : {"g1_legacy_r4", "g1_legacy_b6tune"}) {
            const ProfileState lp = ProfileState::from_json(PyObj::parse(text(dir + "/" + name + ".nvprof.json")));
            QCOMPARE(lp.channels, std::string("YCbCr"));
            QCOMPARE(lp.effective().dump(1), text(dir + "/" + name + ".ui_profile.json"));
            QCOMPARE(lp.to_json().get("format").as_str(), std::string("nv_repro_profile"));
        }
        const ProfileState lr = ProfileState::from_json(PyObj::parse(text(dir + "/g1_legacy_r4.nvprof.json")));
        win->state->set_profile(lr, "G1 legacy");
        CurveEditor* ed = win->profile_panel->curve;
        QCOMPARE(ed->channels, std::string("YCbCr"));
        QVERIFY(ed->target_buttons.count("Y") && ed->target_buttons.count("Cr") && ed->target_buttons.count("Cb"));
        const ProfileState rr = reset_fine_tune(lr);
        QCOMPARE(rr.channels, std::string("RGB"));
        QVERIFY(rr.effective().get("curve") == lr.base.get("ui_pre_aft").get("curve"));
        win->state->undo->undo();
        win->state->undo->undo();
        (void)aft;
    }

    void cleanupTestCase() {
        if (win) {
            win->close();
            delete win;
        }
    }
};

int main(int argc, char** argv) {
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QApplication::setApplicationName("nv_ui_qt");
    SelfTest t;
    return QTest::qExec(&t, argc, argv);
}

#include "selftest.moc"
