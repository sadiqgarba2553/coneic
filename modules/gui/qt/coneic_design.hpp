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
#include <QProxyStyle>
#include <QPainter>
#include <QStyleOption>
#include <QCheckBox>
#include <QGroupBox>
#ifdef Q_OS_WIN
# include <windows.h>
#endif

namespace ConeicDesign {
class ReferenceStyle : public QProxyStyle
{
public:
    ReferenceStyle() : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))) {}
    int pixelMetric(PixelMetric m, const QStyleOption *o=nullptr, const QWidget *w=nullptr) const override
    {
        if(w && (qobject_cast<const QCheckBox*>(w) || qobject_cast<const QGroupBox*>(w))) {
            if(m==PM_IndicatorWidth) return 46;
            if(m==PM_IndicatorHeight) return 22;
        }
        if(m==PM_ScrollBarExtent)return 10;
        return QProxyStyle::pixelMetric(m,o,w);
    }
    void drawPrimitive(PrimitiveElement element,const QStyleOption *o,QPainter *p,const QWidget *w=nullptr) const override
    {
        if(element==PE_IndicatorCheckBox && w && (qobject_cast<const QCheckBox*>(w) || qobject_cast<const QGroupBox*>(w))) {
            p->save();p->setRenderHint(QPainter::Antialiasing);
            const bool on=o->state&State_On, mixed=o->state&State_NoChange;
            const bool dark=o->palette.color(QPalette::Window).lightness()<128;
            const QColor accent=o->palette.color(QPalette::Highlight);
            QRectF r=o->rect.adjusted(1,2,-1,-2);
            if(!(o->state&State_Enabled))p->setOpacity(.4);
            p->setPen(Qt::NoPen);p->setBrush(on?accent.darker(180):(dark?QColor("#292929"):QColor("#dddddd")));
            p->drawRoundedRect(r,3,3);
            QRectF knob(r.left()+2,r.top()+2,14,r.height()-4);
            if(on)knob.moveRight(r.right()-2);
            p->setBrush(on?accent:(dark?QColor("#707070"):QColor("#999999")));
            p->drawRoundedRect(knob,2,2);
            QFont f=w->font();f.setPixelSize(8);p->setFont(f);
            p->setPen(on?accent:o->palette.color(QPalette::WindowText));
            QRectF caption=r.adjusted(on?0:16,0,on?-16:0,0);
            p->drawText(caption,Qt::AlignCenter,mixed?QStringLiteral("—"):(on?QStringLiteral("ON"):QStringLiteral("OFF")));
            if(o->state&State_HasFocus){p->setBrush(Qt::NoBrush);p->setPen(QPen(accent,1));p->drawRoundedRect(o->rect.adjusted(0,0,-1,-1),3,3);}
            p->restore();return;
        }
        QProxyStyle::drawPrimitive(element,o,p,w);
    }
    QIcon standardIcon(StandardPixmap pixmap,const QStyleOption *o=nullptr,const QWidget *w=nullptr) const override
    {
        QString asset;
        switch(pixmap) {
        case SP_DirIcon:case SP_DirOpenIcon:case SP_DirClosedIcon:asset="folder.svg";break;
        case SP_FileIcon:asset="menu-file.svg";break;
        case SP_DialogCloseButton:case SP_TitleBarCloseButton:case SP_DialogCancelButton:asset="close.svg";break;
        case SP_MediaPlay:asset="play.svg";break;
        case SP_MediaPause:asset="pause.svg";break;
        case SP_MediaSkipForward:asset="next.svg";break;
        case SP_MediaSkipBackward:asset="previous.svg";break;
        case SP_FileDialogDetailedView:case SP_FileDialogListView:asset="library-view.svg";break;
        case SP_ArrowBack:case SP_ArrowLeft:asset="library-back.svg";break;
        case SP_ArrowForward:case SP_ArrowRight:asset="library-forward.svg";break;
        case SP_MessageBoxInformation:case SP_MessageBoxQuestion:asset="settings-info.svg";break;
        default:break;
        }
        return asset.isEmpty()?QProxyStyle::standardIcon(pixmap,o,w):QIcon(":/coneic/"+asset);
    }
};
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

inline bool systemDark()
{
#ifdef Q_OS_WIN
    QSettings system(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),QSettings::NativeFormat);
    return !system.value(QStringLiteral("AppsUseLightTheme"),1).toBool();
#else
    return QApplication::palette().color(QPalette::Window).lightness()<128;
#endif
}

inline void apply(QApplication &app, bool dark, bool useDefaultStyle)
{
    // Keep native high-contrast colors and focus cues intact.
    if (highContrast())
        return;
    app.setProperty("coneicDark",dark);

    if (useDefaultStyle)
        app.setStyle(new ReferenceStyle);

    QFont font = app.font();
    static const bool registered=[](){
        for(const QString &name:QStringList({"Inter-Regular.ttf","Inter-Medium.ttf","Inter-SemiBold.ttf"}))
            QFontDatabase::addApplicationFont(":/coneic/fonts/"+name);
        return true;
    }();
    Q_UNUSED(registered);
    const QString family = QStringLiteral("Inter");
    if (QFontDatabase().families().contains(family))
        font.setFamily(family);
    else
        font.setFamily(QStringLiteral("Segoe UI"));
    // Respect larger user-selected text sizes.
    if (font.pointSizeF() < 10.0)
        font.setPointSizeF(10.0);
    app.setFont(font);

    const QColor surface = dark ? QColor("#101010") : QColor("#f5f5f5");
    const QColor raised = dark ? QColor("#202020") : QColor("#ffffff");
    const QColor field = dark ? QColor("#090909") : QColor("#ffffff");
    const QColor text = dark ? QColor("#f5f5f7") : QColor("#1d1d1f");
    const QColor secondary = dark ? QColor("#a1a1aa") : QColor("#5e5e67");
    const QColor accent = dark ? QColor("#d37918") : QColor("#a95200");
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
    app.setStyleSheet(stylesheet + QStringLiteral(
        "QPushButton, QToolButton { border-radius: 3px; }"
        "QPushButton { border: 1px solid transparent; min-height: 22px; }"
        "QToolButton { padding: 3px; }"
        "QPushButton:focus, QToolButton:focus, QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid %1; }"
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { border-radius: 3px; border: 1px solid %2; padding: 5px 8px; }"
        "QGroupBox { border: 0; border-radius: 3px; margin-top: 22px; padding: 12px 8px 8px; }"
        "QGroupBox::title { left: 8px; color: %3; }"
        "QTabWidget::pane { border: 0; border-top: 1px solid %2; border-radius: 0; }"
        "QTabBar::tab { border-radius: 3px; margin: 3px; padding: 7px 12px; color: %3; }"
        "QTabBar::tab:selected { color: %1; }"
        "QCheckBox, QRadioButton { spacing: 9px; min-height: 24px; }"
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }"
        "QScrollBar:horizontal { background: transparent; height: 8px; margin: 0; }"
        "QScrollBar::handle { background: %2; min-height: 28px; min-width: 28px; border-radius: 4px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }"
        "QSlider::groove:horizontal { background: %2; height: 5px; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: %1; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %1; width: 12px; margin: -4px 0; border-radius: 3px; }"
        "QWidget#ConeicSettingsSidebar { background: %4; border-right: 1px solid %2; }"
        "QWidget#ConeicHome { background: %4; }"
        "QWidget#ConeicLibrarySidebar { background: %4; border-right: 1px solid %2; }"
        "QLabel#ConeicBrand { color: %1; font-size: 20px; font-weight: 600; padding: 0 8px 18px; }"
        "QLabel#ConeicHomeTitle { font-size: 24px; font-weight: 600; }"
        "QListWidget#ConeicLibraryNavigation { background: transparent; border: 0; outline: 0; }"
        "QListWidget#ConeicLibraryNavigation::item { padding: 0 8px; margin: 1px 0; border-radius: 3px; color: %3; }"
        "QListWidget#ConeicLibraryNavigation::item:selected { background: %5; color: %1; }"
        "QListWidget#ConeicMediaCards { background: transparent; border: 0; }"
        "QListWidget#ConeicMediaCards::item { border-radius: 3px; padding: 4px; }"
        "QListWidget#ConeicMediaCards::item:selected { background: %5; color: %1; }"
        "QListWidget#ConeicMomentCards, QListWidget#ConeicStudyNotes, QListWidget#ConeicCollectionMedia { background: transparent; border: 0; }"
        "QListWidget#ConeicMomentCards::item, QListWidget#ConeicStudyNotes::item, QListWidget#ConeicCollectionMedia::item { background: %4; border-radius: 6px; padding: 8px 12px; }"
        "QListWidget#ConeicMomentCards::item:selected, QListWidget#ConeicStudyNotes::item:selected, QListWidget#ConeicCollectionMedia::item:selected { background: %5; color: %3; border: 1px solid %1; }"
        "QPlainTextEdit#ConeicNoteText { background: %4; border: 1px solid %2; border-radius: 6px; padding: 10px; }"
        "QPushButton#ConeicCaptureMoment, QPushButton#ConeicSaveNote { background: %1; color: %8; border: 1px solid %1; }"
        "QLabel#ConeicLibraryEmpty { color: %3; font-size: 15px; }"
        "QListWidget#ConeicSettingsNavigation { background: transparent; border: 0; outline: 0; }"
        "QListWidget#ConeicSettingsNavigation::item { padding: 0 8px; margin: 1px 0; border-radius: 3px; color: %3; }"
        "QListWidget#ConeicSettingsNavigation::item:selected { background: %5; color: %1; }"
        "QLabel#ConeicSettingsHeading { font-size: 18px; padding: 2px 0 10px; }"
        "QGroupBox#ConeicSettingsMode { margin: 0; padding: 0; }"
        "QComboBox::drop-down { border: 0; width: 22px; background: transparent; }"
        "QLabel#ConeicSectionTitle { font-weight: 600; padding: 14px 0 6px; }"
        "QToolButton#ConeicAdvancedDisclosure { background: %4; padding: 7px; margin: 10px 0; }"
        "QFrame#ConeicTransport, QFrame#ConeicSeekRow { background: %4; border: 0; }"
        "QToolButton#ConeicPlayButton, QPushButton#ConeicOpenFile, QPushButton#ConeicResumeContinue { background: %1; color: %8; border: 1px solid %1; }"
        "QToolButton#ConeicPlayButton:hover, QPushButton#ConeicOpenFile:hover { background: %6; }"
        "QToolButton#ConeicPlayButton:pressed, QPushButton#ConeicOpenFile:pressed { background: %7; }"
        "QMenuBar { padding: 1px 8px; }"
        "QMenuBar::item { padding: 5px 9px; border-radius: 3px; }"
        "QMenu { border: 1px solid %2; padding: 5px; }"
        "QMenu::item { border-radius: 3px; padding: 8px 28px 8px 20px; }"
    ).arg(accent.name(),border,secondary.name(),raised.name(),dark?"#342718":"#fff0e0",accent.lighter(115).name(),accent.darker(115).name(),accentText.name()));

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
