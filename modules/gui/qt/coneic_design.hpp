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
#include "coneic_glass_material.hpp"
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
            if(m==PM_IndicatorWidth) return 36;
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
            p->drawRoundedRect(r,10,10);
            QRectF knob(r.left()+2,r.top()+2,14,r.height()-4);
            if(on)knob.moveRight(r.right()-2);
            p->setBrush(on?accent:(dark?QColor("#707070"):QColor("#999999")));
            p->drawRoundedRect(knob,7,7);
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
    if (highContrast()) {
        app.setProperty("coneicGlassEnabled",false);
        app.setProperty("coneicTransparency",false);
        app.setStyleSheet(QString());
        return;
    }
    app.setProperty("coneicDark",dark);
    bool transparency=true;
#ifdef Q_OS_WIN
    QSettings material(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),QSettings::NativeFormat);
    transparency=material.value(QStringLiteral("EnableTransparency"),1).toBool();
#endif
    const int materialMode=app.property("coneicMaterialMode").toInt();
    if(materialMode==1)transparency=true;
    else if(materialMode==2)transparency=false;
    app.setProperty("coneicTransparency",transparency);
    app.setProperty("coneicGlassEnabled",true);
    ConeicGlass::installAmbient(app);

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

    const QColor surface = dark ? QColor("#101e2b") : QColor("#edf2f7");
    const QColor raised = dark ? QColor("#20313f") : QColor("#f5f8fc");
    const QColor field = dark ? QColor("#12202d") : QColor("#ffffff");
    const QColor text = dark ? QColor("#f5f5f7") : QColor("#1d1d1f");
    const QColor secondary = dark ? QColor("#aebfcd") : QColor("#536a7c");
    const QColor accent = dark ? QColor("#b7dcef") : QColor("#326a89");
    const QColor accentText = dark ? QColor("#142331") : QColor("#ffffff");
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
    app.setStyleSheet(ConeicGlass::stylesheet(dark,transparency));
    for(auto window:app.topLevelWidgets())ConeicGlass::nativeFrame(window,dark);

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
