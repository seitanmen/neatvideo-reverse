// look - the application look of the C++ UI: the Fusion style with the grey palette of the genuine Neat Video 6.1.3
// window (sandbox/omo/wp_qtui_style/ref/genuine.png), the same on macOS and on offscreen (the self-test).
//
// The style (a QProxyStyle on Fusion) draws the menu indicator of a tool button as a small arrow inside the button at
// the bottom right, as the genuine buttons do: for InstantPopup / DelayedPopup (Fusion already does), for
// MenuButtonPopup (Fusion draws a separate arrow section; here the bevel covers the whole button and the right
// PM_MenuButtonIndicator pixels still open the menu), and for a button whose text ends in " ▾" (the ▾ is not drawn
// as text; the text itself is unchanged).  Property nv_arrow = false on a button: no indicator.
// Slider grooves are not filled with the highlight colour (genuine: plain grey groove).
#pragma once

class QApplication;

namespace nvqt {

void apply_look(QApplication& app);

// the genuine colours (8-bit sRGB from genuine.png)
inline constexpr const char* LOOK_MENUBAR = "#3a3a3a";   // menu bar row (58)
inline constexpr const char* LOOK_TABROW = "#484848";    // page tab row, unselected tab (72)
inline constexpr const char* LOOK_WINDOW = "#595959";    // page area, selected tab (89)

}  // namespace nvqt
