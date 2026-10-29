// Side-panel building blocks in the genuine style (port of repro/nv_ui_qt/widgets/rows.py): grey section
// headers ("▼ Name ...... Enable ☑") and value rows ("Label  (dot / band pattern)  slider  value"), the value
// underlined with dots and editable by a click.  A row can drive several keys at once (Frequencies rows write
// Y, Cr and Cb).  Names follow the Python module.
#pragma once

#include <QAction>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QWidget>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "state.h"

namespace nvqt {

QColor ch_color(const std::string& c);           // CH_COLORS (Y, Cr, Cb, R, G, B)
int band_scale(const std::string& band);         // BAND_SCALE
extern const char* HEADER_CSS;
extern const char* HEADER_TEXT;
extern const char* ACTIVE_TITLE;

// FORMATS: (to display, from display, text, slider scale) of a row's display format
struct Format {
    double (*to_d)(double);
    double (*from_d)(double);
    QString (*text)(double);
    int sc;
};
const Format& format(const std::string& fmt);    // "amount", "rel", "inv", "tune", "frames", "float", "range"

QPixmap dot_pixmap(const std::string& c, int size = 14);
QIcon glyph_icon(const QString& text, QColor color = QColor(220, 220, 220), int size = 20, bool bold = false);
QIcon info_icon(int size = 18);
QIcon pin_icon(int size = 18);
QIcon square_icon(QColor color, int size = 12);
QIcon triangle_icon(QColor color = QColor(190, 190, 190), int size = 12);
QPixmap band_pixmap(const std::string& band, int w = 22, int h = 16);   // only the 22 x 16 pictures are embedded

class ValueLabel : public QLabel {
    Q_OBJECT
public:
    explicit ValueLabel(QWidget* parent = nullptr);
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
};

class Row : public QWidget {
    Q_OBJECT
public:
    // keys: one or several; label: std::nullopt = the spec's label, "" = no label; spec: nullptr =
    // spec(keys[0]); srange = (lo, hi) of the slider in display units (the exact entry keeps the spec range)
    Row(std::vector<std::string> keys, std::optional<QString> label = std::nullopt, const std::string& fmt = "amount",
        const std::string& deco = "", std::pair<int, int> steps = {5, 20}, int indent = 0, const Spec* spec_ = nullptr,
        std::optional<std::pair<double, double>> srange = std::nullopt, QWidget* parent = nullptr);

    double value() const { return internal; }
    void set_value(double v, bool mixed = false);

    std::vector<std::string> keys;
    std::string key;
    Spec spec;
    Format fmtf;
    double internal;
    QLabel* label;
    QLabel* deco;
    QSlider* slider;
    QStackedWidget* stack;
    ValueLabel* val;
    QDoubleSpinBox* spin;
    QToolButton* reset;
    bool implemented;

signals:
    void edited(QString key, double value);
    void editedMany(QVariantMap values);
    void editStarted();

private:
    void action_(int a);
    void show_(double v, bool mixed);
    void emit_(double v);
    void from_slider_(int iv);
    void edit_exact_();
    void exact_done_();
    void reset_();
    bool block_ = false;
};

class EnableCheck : public QCheckBox {
    Q_OBJECT
public:
    explicit EnableCheck(const std::string& key, QWidget* parent = nullptr);
    bool value() const { return isChecked(); }
    void set_value(bool v);

    std::string key;
    Spec spec;
    QLabel* na_label = nullptr;           // the header / row label that shows "(n/a)" for this box

signals:
    void edited(QString key, bool value);
    void editStarted();

private:
    bool block_ = false;
};

class ChoiceRow : public QWidget {
    Q_OBJECT
public:
    ChoiceRow(const std::string& key, const QString& label, const std::vector<QString>& names = {}, int indent = 0,
              QWidget* parent = nullptr);
    QString value() const;
    void set_value(const QString& v);

    std::string key;
    Spec spec;
    QLabel* label;
    QComboBox* box;
    std::vector<std::string> choices;

signals:
    void edited(QString key, QString value);
    void editStarted();

private:
    bool block_ = false;
};

class BeginRow : public QWidget {
    Q_OBJECT
public:
    BeginRow(const QString& title, const QString& tip = "", bool checkable = true, bool enabled = true, int indent = 0,
             bool dim = false, QWidget* parent = nullptr);
    void set_checked(bool on);

    QString name;
    bool dim;
    QLabel* title;
    QPushButton* button;

signals:
    void toggled(bool on);

private:
    void look_(bool on);
};

class Header : public QWidget {
    Q_OBJECT
public:
    Header(const QString& title, EnableCheck* enable = nullptr, bool expanded = true, bool collapsible = true,
           int level = 0, QWidget* inline_ = nullptr, bool reset = false, const QString& help_tip = QString(),
           QWidget* parent = nullptr);
    void toggle();
    void set_expanded(bool on);
    QWidget* add(QWidget* w);

    QWidget* band;
    QToolButton* arrow;
    bool collapsible;
    QLabel* title;
    QWidget* inline_w;
    EnableCheck* enable;
    QToolButton* reset_btn = nullptr;
    QWidget* body;
    QVBoxLayout* lay;
    QString title_text;

signals:
    void resetClicked();

protected:
    bool eventFilter(QObject* o, QEvent* e) override;
};

QWidget* separator();

class IconBar : public QWidget {
    Q_OBJECT
public:
    explicit IconBar(const QString& what, QWidget* parent = nullptr);
    // undo / redo: the QUndoStack actions; stack: the QUndoStack whose history the ▾ menus list
    void bind_undo(QAction* undo, QAction* redo, QUndoStack* stack = nullptr);

    QToolButton *open, *save, *undo, *redo, *reset, *default_, *info;
    QMenu* default_menu;

private:
    static void history_(QMenu* menu, QUndoStack* stack, bool back);
};

QLabel* panel_title(const QString& text);
std::pair<QLabel*, QLabel*> guide_labels();     // 'How to use: Quick Start Guide.' + mode comment
QString mode_text(bool advanced);

}  // namespace nvqt
