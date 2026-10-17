/*****************************************************************************
 * coneic_design.hpp: Coneic presentation for the VLC Qt interface
 * Copyright (C) 2026 Coneic contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Presentation only. VLC action mappings, menus, shortcuts, media/device
 * handling, extension hooks and advanced settings remain upstream code.
 *****************************************************************************/
#ifndef CONEIC_DESIGN_HPP
#define CONEIC_DESIGN_HPP

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#ifdef Q_OS_WIN
# include <windows.h>
#endif

namespace ConeicDesign {
inline bool highContrast()
{
#ifdef Q_OS_WIN
    HIGHCONTRASTW settings = {};
    settings.cbSize = sizeof(settings);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(settings), &settings, 0))
        return (settings.dwFlags & HCF_HIGHCONTRASTON) != 0;
#endif
    return false;
}

inline bool motionEnabled()
{
#ifdef Q_OS_WIN
    BOOL enabled = TRUE;
    if (SystemParametersInfoW(0x1042 /* SPI_GETCLIENTAREAANIMATION; Vista+ */, 0, &enabled, 0))
        return enabled != FALSE;
#endif
    return true;
}

inline void apply(QApplication &app, bool dark, bool useDefaultStyle)
{
    // Keep native high-contrast colors and focus cues intact.
    if (highContrast())
        return;

    if (useDefaultStyle)
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QFont font = app.font();
    const QString family = QStringLiteral("Segoe UI Variable Text");
    if (QFontDatabase().families().contains(family))
        font.setFamily(family);
    else
        font.setFamily(QStringLiteral("Segoe UI"));
    // Respect larger user-selected text sizes.
    if (font.pointSizeF() < 10.0)
        font.setPointSizeF(10.0);
    app.setFont(font);

    const QColor surface = dark ? QColor("#1c1c1e") : QColor("#f5f5f7");
    const QColor raised = dark ? QColor("#2c2c2e") : QColor("#ffffff");
    const QColor field = dark ? QColor("#141416") : QColor("#ffffff");
    const QColor text = dark ? QColor("#f5f5f7") : QColor("#1d1d1f");
    const QColor secondary = dark ? QColor("#a1a1aa") : QColor("#5e5e67");
    const QColor accent = dark ? QColor("#ff8a3d") : QColor("#b84b00");
    const QColor accentText = dark ? QColor("#20150e") : QColor("#ffffff");
    const QString border = dark ? QStringLiteral("#48484d") : QStringLiteral("#c7c7cf");
    const QString hover = dark ? QStringLiteral("#3a3a3e") : QStringLiteral("#e8e8ed");
    const QString press = dark ? QStringLiteral("#48484e") : QStringLiteral("#d8d8e1");
    QPalette palette = app.palette();
    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive})
    {
        palette.setColor(group, QPalette::Window, surface);
        palette.setColor(group, QPalette::WindowText, text);
        palette.setColor(group, QPalette::Base, field);
        palette.setColor(group, QPalette::AlternateBase, raised);
        palette.setColor(group, QPalette::Text, text);
        palette.setColor(group, QPalette::Button, raised);
        palette.setColor(group, QPalette::ButtonText, text);
        palette.setColor(group, QPalette::Highlight, accent);
        palette.setColor(group, QPalette::HighlightedText, accentText);
        palette.setColor(group, QPalette::Link, accent);
        palette.setColor(group, QPalette::ToolTipBase, raised);
        palette.setColor(group, QPalette::ToolTipText, text);
    }
    palette.setColor(QPalette::Disabled, QPalette::Text, secondary);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, secondary);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, secondary);
    app.setPalette(palette);

    // Native Qt slider tracking, keyboard navigation, focus and accessibility
    // remain in place. Press/hover feedback never waits for a transition.
    const QString stylesheet = QStringLiteral(
        "QMainWindow, QDialog { background: %1; color: %2; }"
        "QMenuBar { background: %1; padding: 5px 8px; spacing: 3px; }"
        "QMenuBar::item { padding: 6px 10px; border-radius: 5px; }"
        "QMenuBar::item:selected { background: %3; }"
        "QMenu { background: %4; border: 1px solid %5; padding: 5px; }"
        "QMenu::item { padding: 7px 26px 7px 14px; border-radius: 5px; }"
        "QMenu::item:selected { background: %6; color: %9; }"
        "QMenu::separator { height: 1px; background: %5; margin: 5px 8px; }"
        "QPushButton { background: %4; color: %2; border: 1px solid %5;"
        " border-radius: 7px; padding: 6px 14px; min-height: 20px; }"
        "QPushButton:hover, QToolButton:hover { background: %3; }"
        "QPushButton:pressed, QToolButton:pressed { background: %7; }"
        "QPushButton:default { background: %6; color: %9; border-color: %6; }"
        "QPushButton:focus, QToolButton:focus, QLineEdit:focus, QComboBox:focus,"
        " QSpinBox:focus, QDoubleSpinBox:focus { border: 2px solid %6; }"
        "QToolButton { border: 1px solid transparent; border-radius: 7px; padding: 5px; }"
        "QToolButton:checked { background: %3; border-color: %6; }"
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: %8;"
        " color: %2; border: 1px solid %5; border-radius: 6px; padding: 5px; }"
        "QComboBox QAbstractItemView { background: %4; color: %2;"
        " selection-background-color: %6; selection-color: %9; }"
        "QTreeView, QListView, QTableView { background: %8;"
        " alternate-background-color: %4; border: 0; outline: 0; }"
        "QTreeView::item, QListView::item { min-height: 25px; padding: 3px; }"
        "QTreeView::item:selected, QListView::item:selected { background: %6; color: %9; }"
        "QHeaderView::section { background: %1; color: %2; border: 0;"
        " border-bottom: 1px solid %5; padding: 8px; }"
        "QTabWidget::pane { border: 1px solid %5; border-radius: 8px; }"
        "QTabBar::tab { background: %1; padding: 8px 15px; border-radius: 5px; }"
        "QTabBar::tab:selected { background: %4; color: %2; }"
        "QGroupBox { border: 1px solid %5; border-radius: 8px;"
        " margin-top: 14px; padding: 12px 9px 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; }"
        "QStatusBar { background: %1; padding: 4px; }"
        "QToolTip { background: %4; color: %2; border: 1px solid %5;"
        " padding: 7px; border-radius: 5px; }"
    ).arg(surface.name(), text.name(), hover, raised.name(), border,
          accent.name(), press, field.name(), accentText.name());
    app.setStyleSheet(stylesheet);

    // Independently honor Windows' reduced-motion setting; there are no
    // timer-delayed actions, parallax or looping decorative animations.
    if (!motionEnabled())
    {
        app.setEffectEnabled(Qt::UI_AnimateMenu, false);
        app.setEffectEnabled(Qt::UI_FadeMenu, false);
        app.setEffectEnabled(Qt::UI_AnimateCombo, false);
        app.setEffectEnabled(Qt::UI_FadeTooltip, false);
    }
}
}
#endif
