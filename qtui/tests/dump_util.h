// dump_util - helpers shared by the comparison drivers qtui_dump_n3c and qtui_ofx_drv (header only): the displayed
// state of form widgets, the menu bar, and the N3c main-window strings, as qtui/tools/py_dump_n3c.py writes them.
#pragma once

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>

#include <functional>
#include <string>

#include "filter_panel.h"
#include "mainwindow.h"
#include "pyobj.h"

namespace nvqt::dump {

inline PyObj S(const QString& s) { return PyObj::str(s.toStdString()); }

inline PyObj widget_info(QWidget* w) {
    PyObj d = PyObj::dict();
    if (!w) {
        d.set("kind", PyObj::str("none"));
        return d;
    }
    d.set("enabled", PyObj::boolean(w->isEnabled()));
    d.set("tip", S(w->toolTip()));
    if (auto* ds = qobject_cast<QDoubleSpinBox*>(w)) {
        d.set("kind", PyObj::str("dspin"));
        d.set("value", PyObj::real(ds->value()));
        d.set("text", S(ds->text()));
        d.set("min", PyObj::real(ds->minimum()));
        d.set("max", PyObj::real(ds->maximum()));
        d.set("step", PyObj::real(ds->singleStep()));
    } else if (auto* sp = qobject_cast<QSpinBox*>(w)) {
        d.set("kind", PyObj::str("spin"));
        d.set("value", PyObj::integer(sp->value()));
        d.set("text", S(sp->text()));
        d.set("min", PyObj::integer(sp->minimum()));
        d.set("max", PyObj::integer(sp->maximum()));
    } else if (auto* cb = qobject_cast<QComboBox*>(w)) {
        d.set("kind", PyObj::str("combo"));
        d.set("text", S(cb->currentText()));
        PyObj items = PyObj::list(), data = PyObj::list();
        for (int i = 0; i < cb->count(); ++i) {
            items.append(S(cb->itemText(i)));
            const QVariant v = cb->itemData(i);
            data.append(v.isValid() ? S(v.toString()) : PyObj::none());
        }
        d.set("items", items);
        d.set("data", data);
    } else if (auto* ch = qobject_cast<QCheckBox*>(w)) {
        d.set("kind", PyObj::str("check"));
        d.set("text", S(ch->text()));
        d.set("checked", PyObj::boolean(ch->isChecked()));
    } else if (auto* le = qobject_cast<QLineEdit*>(w)) {
        d.set("kind", PyObj::str("line"));
        d.set("text", S(le->text()));
    } else if (auto* lb = qobject_cast<QLabel*>(w)) {
        d.set("kind", PyObj::str("label"));
        d.set("text", S(lb->text()));
    } else if (auto* bb = qobject_cast<QDialogButtonBox*>(w)) {
        d.set("kind", PyObj::str("buttons"));
        PyObj t = PyObj::list();
        for (QAbstractButton* b : bb->buttons()) t.append(S(b->text()));
        d.set("texts", t);
    } else {
        d.set("kind", PyObj::str(w->metaObject()->className()));
    }
    return d;
}

inline PyObj form_rows(QFormLayout* f) {
    PyObj rows = PyObj::list();
    for (int i = 0; i < f->rowCount(); ++i) {
        QLayoutItem* li = f->itemAt(i, QFormLayout::LabelRole);
        QLayoutItem* fi = f->itemAt(i, QFormLayout::FieldRole);
        if (!fi) fi = f->itemAt(i, QFormLayout::SpanningRole);
        PyObj r = PyObj::dict();
        auto* lw = li ? qobject_cast<QLabel*>(li->widget()) : nullptr;
        r.set("label", lw ? S(lw->text()) : PyObj::none());
        const PyObj w = widget_info(fi ? fi->widget() : nullptr);
        for (const auto& k : w.keys()) r.set(k, w.get(k));
        rows.append(r);
    }
    return rows;
}

inline PyObj menus(MainWindow& win) {
    PyObj out = PyObj::list();
    for (QAction* top : win.menuBar()->actions()) {
        PyObj m = PyObj::dict();
        m.set("title", S(top->text()));
        PyObj items = PyObj::list();
        std::function<void(QMenu*, const std::string&)> walk = [&](QMenu* menu, const std::string& pre) {
            for (QAction* a : menu->actions()) {
                PyObj it = PyObj::dict();
                it.set("text", PyObj::str(pre + a->text().toStdString()));
                it.set("sep", PyObj::boolean(a->isSeparator()));
                it.set("enabled", PyObj::boolean(a->isEnabled()));
                it.set("checkable", PyObj::boolean(a->isCheckable()));
                it.set("shortcut", S(a->shortcut().toString()));
                items.append(it);
                if (a->menu()) walk(a->menu(), pre + a->text().toStdString() + " > ");
            }
        };
        if (top->menu()) walk(top->menu(), "");
        m.set("items", items);
        out.append(m);
    }
    return out;
}

inline PyObj extras(MainWindow& win) {
    PyObj d = PyObj::dict();
    QWidget* box = win.image_info();
    auto* mb = qobject_cast<QMessageBox*>(box);
    d.set("perf", S(win.filter_panel->perf->text()));
    d.set("image_info", mb ? S(mb->text()) : PyObj::none());
    if (box) box->close();
    const auto ef = win.export_filter();
    PyObj e = PyObj::list();
    e.append(S(ef.first));
    e.append(S(ef.second));
    d.set("export_filter", e);
    d.set("title", S(win.windowTitle()));
    d.set("depth_used", PyObj::str(win.depth_used));
    d.set("msg", S(win.msg->text()));
    return d;
}

inline std::string state_with(MainWindow& win, const PyObj& ex) {
    PyObj d = PyObj::parse(win.dump_state().toStdString());
    for (const auto& k : ex.keys()) d.set(k, ex.get(k));
    return d.dump(1);
}

}  // namespace nvqt::dump
