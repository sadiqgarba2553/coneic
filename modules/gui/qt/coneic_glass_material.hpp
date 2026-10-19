/* Coneic glass materials. Copyright (C) 2026 Coneic contributors.
 * SPDX-License-Identifier: GPL-2.0-or-later. Native Qt presentation only. */
#ifndef CONEIC_GLASS_MATERIAL_HPP
#define CONEIC_GLASS_MATERIAL_HPP
#include <QApplication>
#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QLibrary>
#include <QDir>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace ConeicGlass {
inline void nativeFrame(QWidget *widget,bool dark) {
#ifdef Q_OS_WIN
    using SetAttribute=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);
    static QLibrary dwm(QDir::toNativeSeparators(QString::fromLocal8Bit(qgetenv("SystemRoot"))+"/System32/dwmapi.dll"));
    static auto set=reinterpret_cast<SetAttribute>(dwm.resolve("DwmSetWindowAttribute"));
    if(set && widget->windowHandle() && !(widget->windowFlags()&Qt::FramelessWindowHint)) {
        const HWND hwnd=reinterpret_cast<HWND>(widget->winId());const BOOL immersive=dark;const int round=2;
        const COLORREF caption=dark?RGB(20,32,45):RGB(233,239,244);
        set(hwnd,20,&immersive,sizeof(immersive)); // DWMWA_USE_IMMERSIVE_DARK_MODE
        set(hwnd,33,&round,sizeof(round)); // DWMWA_WINDOW_CORNER_PREFERENCE
        set(hwnd,35,&caption,sizeof(caption)); // DWMWA_CAPTION_COLOR, supported on Windows 11
    }
#else
    Q_UNUSED(widget);Q_UNUSED(dark);
#endif
}
inline void ambient(QPainter &p,const QRectF &r,bool dark)
{
    p.fillRect(r,dark?QColor("#0b1420"):QColor("#e9eff4"));
    QLinearGradient base(r.topLeft(),r.bottomRight());
    base.setColorAt(0,dark?QColor("#1c2b3d"):QColor("#edf2f8"));
    base.setColorAt(.55,dark?QColor("#10222c"):QColor("#dae9ee"));
    base.setColorAt(1,dark?QColor("#191b2e"):QColor("#ece6ee"));
    p.fillRect(r,base);
    QRadialGradient light(QPointF(r.right()*.88,r.top()+r.height()*.12),r.width()*.8);
    light.setColorAt(0,dark?QColor(121,165,196,45):QColor(255,255,255,170));
    light.setColorAt(1,Qt::transparent);p.fillRect(r,light);
}
class AmbientFilter : public QObject {
public:
    explicit AmbientFilter(QObject *parent):QObject(parent){}
    bool eventFilter(QObject *object,QEvent *event) override {
        auto widget=qobject_cast<QWidget*>(object);
        if(!qApp->property("coneicGlassEnabled").toBool())return false;
        if(event->type()==QEvent::Show && widget && widget->isWindow())nativeFrame(widget,qApp->property("coneicDark").toBool());
        if(event->type()!=QEvent::Paint)return false;
        if(widget && ((widget->isWindow() && (widget->inherits("QMainWindow") || qobject_cast<QDialog*>(widget))) || widget->objectName()=="ConeicHome")) {
            QPainter p(widget);ambient(p,widget->rect(),qApp->property("coneicDark").toBool());
        }
        return false;
    }
};
inline void installAmbient(QApplication &app)
{
    if(app.property("coneicAmbientInstalled").toBool())return;
    app.installEventFilter(new AmbientFilter(&app));app.setProperty("coneicAmbientInstalled",true);
}
inline QString stylesheet(bool dark,bool transparency)
{
    QString s=QString::fromUtf8(R"GLASS(
QMainWindow, QDialog { background: transparent; color: @TEXT; }
QWidget { color: @TEXT; }
QLabel { background: transparent; border: 0; }
QMenuBar, QWidget#ConeicChrome { background: transparent; padding: 3px 10px; }
QMenuBar::item { padding: 7px 12px; border-radius: 10px; }
QMenuBar::item:selected { background: @HOVER; }
QMenu { background: @MENU; border: 1px solid @LINE; border-radius: 14px; padding: 7px; }
QMenu::item { padding: 9px 28px 9px 14px; border-radius: 8px; }
QMenu::item:selected { background: @HOVER; color: @TEXT; }
QMenu::separator { height: 1px; background: @LINE; margin: 6px 10px; }
QPushButton { background: @SURFACE; border: 1px solid @LINE; border-radius: 11px; padding: 8px 15px; min-height: 22px; }
QPushButton:hover, QToolButton:hover { background: @HOVER; }
QPushButton:pressed, QToolButton:pressed { background: @PRESS; }
QPushButton:default, QPushButton#ConeicOpenFile, QPushButton#ConeicCaptureMoment, QPushButton#ConeicSaveNote, QPushButton#ConeicResumeContinue { background: @PRIMARY; color: @PRIMARYTEXT; border-color: transparent; }
QToolButton { border: 1px solid transparent; border-radius: 10px; padding: 5px; background: transparent; }
QToolButton:checked { background: @HOVER; border-color: @LINE; }
QPushButton:focus, QToolButton:focus, QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid @ACCENT; }
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit, QPlainTextEdit { background: @FIELD; color: @TEXT; border: 1px solid @LINE; border-radius: 10px; padding: 8px 11px; selection-background-color: @ACCENT; selection-color: @PRIMARYTEXT; }
QComboBox::drop-down { border: 0; width: 26px; background: transparent; }
QComboBox QAbstractItemView { background: @MENU; color: @TEXT; selection-background-color: @HOVER; }
QTreeView, QListView, QTableView { background: @FIELD; alternate-background-color: @SURFACE; border: 1px solid @LINE; border-radius: 12px; outline: 0; }
QTreeView::item, QListView::item { min-height: 28px; padding: 5px; }
QTreeView::item:selected, QListView::item:selected { background: @HOVER; color: @TEXT; }
QHeaderView::section { background: @SURFACE; color: @SECONDARY; border: 0; padding: 10px; }
QTabWidget::pane { border: 1px solid @LINE; border-radius: 14px; background: @SURFACE; }
QTabBar::tab { background: transparent; color: @SECONDARY; padding: 9px 14px; margin: 3px; border-radius: 10px; }
QTabBar::tab:selected { background: @HOVER; color: @TEXT; }
QGroupBox { border: 1px solid @LINE; border-radius: 13px; margin-top: 20px; padding: 14px 12px 10px; background: @SURFACE; }
QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 5px; color: @SECONDARY; }
QCheckBox, QRadioButton { spacing: 10px; min-height: 26px; background: transparent; }
QStatusBar { background: transparent; padding: 4px; color: @SECONDARY; }
QToolTip { background: @MENU; color: @TEXT; border: 1px solid @LINE; padding: 8px 11px; border-radius: 8px; }
QScrollArea, QScrollArea > QWidget > QWidget { border: 0; background: transparent; }
QScrollBar:vertical { background: transparent; width: 7px; margin: 2px; }
QScrollBar:horizontal { background: transparent; height: 7px; margin: 2px; }
QScrollBar::handle { background: @LINE; min-height: 28px; min-width: 28px; border-radius: 3px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
QSlider::groove:horizontal { background: @LINE; height: 3px; border-radius: 1px; }
QSlider::sub-page:horizontal { background: @ACCENT; border-radius: 1px; }
QSlider::handle:horizontal { background: @PRIMARY; width: 10px; margin: -4px 0; border-radius: 5px; }
QWidget#ConeicHome { background: transparent; }
QWidget#ConeicLibrarySidebar, QWidget#ConeicSettingsSidebar { background: @SURFACE; border: 1px solid @LINE; border-radius: 17px; }
QLabel#ConeicBrand { color: @TEXT; font-size: 22px; font-weight: 600; padding: 8px 12px 22px; }
QLabel#ConeicHomeTitle { font-size: 27px; font-weight: 600; }
QLabel#ConeicHomeSubtitle { color: @SECONDARY; font-size: 12px; padding: 2px 0 8px; }
QListWidget#ConeicLibraryNavigation, QListWidget#ConeicSettingsNavigation { background: transparent; border: 0; outline: 0; }
QListWidget#ConeicLibraryNavigation::item, QListWidget#ConeicSettingsNavigation::item { padding: 2px 10px; margin: 2px 0; border-radius: 11px; color: @SECONDARY; }
QListWidget#ConeicLibraryNavigation::item:selected, QListWidget#ConeicSettingsNavigation::item:selected { background: @HOVER; color: @TEXT; }
QListWidget#ConeicMediaCards, QListWidget#ConeicMomentCards, QListWidget#ConeicStudyNotes, QListWidget#ConeicCollectionMedia, QListWidget#ConeicCommandResults { background: transparent; border: 0; }
QListWidget#ConeicMediaCards::item { background: transparent; border: 0; }
QListWidget#ConeicMomentCards::item, QListWidget#ConeicStudyNotes::item, QListWidget#ConeicCollectionMedia::item, QListWidget#ConeicCommandResults::item { background: @SURFACE; border: 1px solid @LINE; border-radius: 12px; padding: 10px 13px; margin: 3px; }
QListWidget#ConeicMomentCards::item:selected, QListWidget#ConeicStudyNotes::item:selected, QListWidget#ConeicCollectionMedia::item:selected, QListWidget#ConeicCommandResults::item:selected { background: @HOVER; border-color: @ACCENT; }
QLabel#ConeicLibraryEmpty { color: @SECONDARY; font-size: 16px; }
QLabel#ConeicSettingsHeading { font-size: 22px; font-weight: 600; padding: 5px 0 13px; }
QLabel#ConeicSectionTitle { font-weight: 600; color: @SECONDARY; padding: 17px 0 8px; }
QLabel#ConeicCompanyLink { color: @SECONDARY; }
QGroupBox#ConeicSettingsMode { background: transparent; border: 0; margin: 0; padding: 0; }
QToolButton#ConeicAdvancedDisclosure { background: @SURFACE; border: 1px solid @LINE; padding: 9px; margin: 10px 0; }
QFrame#ConeicTransport, QFrame#ConeicSeekRow { background: @SURFACE; border: 0; }
QWidget#ConeicGlassDock, QWidget#ConeicGlassDock QLabel, QWidget#ConeicGlassDock QFrame { background: transparent; border: 0; color: white; }
QWidget#ConeicGlassDock QSlider::groove:horizontal { background: rgba(255,255,255,55); }
QWidget#ConeicGlassDock QSlider::sub-page:horizontal, QWidget#ConeicGlassDock QSlider::handle:horizontal { background: white; }
)GLASS");
    s.replace("@TEXT",dark?"#f0f4f9":"#182b3a");
    s.replace("@SECONDARY",dark?"#aebfcd":"#536a7c");
    s.replace("@LINE",dark?"rgba(225,241,255,30)":"rgba(75,105,132,42)");
    s.replace("@SURFACE",transparency?(dark?"rgba(213,232,249,12)":"rgba(255,255,255,110)"):(dark?"#1c2a38":"#f2f6fa"));
    s.replace("@FIELD",transparency?(dark?"rgba(5,15,25,70)":"rgba(255,255,255,125)"):(dark?"#12202d":"#ffffff"));
    s.replace("@MENU",dark?"#20313f":"#f0f5fa");
    s.replace("@HOVER",dark?"rgba(210,234,255,25)":"rgba(255,255,255,175)");
    s.replace("@PRESS",dark?"rgba(210,234,255,40)":"rgba(210,226,239,200)");
    s.replace("@PRIMARYTEXT",dark?"#142331":"#ffffff");
    s.replace("@PRIMARY",dark?"#e1edf7":"#234960");
    s.replace("@ACCENT",dark?"#b7dcef":"#326a89");
    return s;
}
class MediaCardDelegate : public QStyledItemDelegate {
public:
    explicit MediaCardDelegate(QObject *parent):QStyledItemDelegate(parent){}
    void paint(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        if(!qApp->property("coneicGlassEnabled").toBool()){QStyledItemDelegate::paint(p,option,index);return;}
        const bool dark=qApp->property("coneicDark").toBool();
        const QColor text=dark?QColor("#edf4fa"):QColor("#203343");
        p->save();p->setRenderHint(QPainter::Antialiasing);
        QRect r=option.rect.adjusted(3,3,-3,-3);
        if(index.data(Qt::UserRole+1).toBool()) {
            QFont font=option.font;font.setWeight(QFont::DemiBold);p->setFont(font);p->setPen(text);
            p->drawText(r.adjusted(3,0,0,0),Qt::AlignLeft|Qt::AlignVCenter,index.data().toString());p->restore();return;
        }
        const bool selected=option.state&QStyle::State_Selected,hover=option.state&QStyle::State_MouseOver;
        QLinearGradient frost(r.topLeft(),r.bottomRight());
        frost.setColorAt(0,dark?QColor(235,247,255,selected?34:hover?24:16):QColor(255,255,255,180));
        frost.setColorAt(1,dark?QColor(190,215,240,7):QColor(255,255,255,80));
        p->setBrush(frost);p->setPen(QPen(selected?QColor("#b7dcef"):(dark?QColor(230,244,255,28):QColor(50,85,110,26)),1));
        p->drawRoundedRect(r,15,15);
        QRect cover(r.left()+9,r.top()+9,r.width()-18,(r.width()-18)*9/16);
        QPainterPath clip;clip.addRoundedRect(cover,10,10);p->save();p->setClipPath(clip);
        p->fillRect(cover,dark?QColor(6,19,29,110):QColor(210,226,238,130));
        QIcon icon=qvariant_cast<QIcon>(index.data(Qt::DecorationRole));QPixmap image=icon.pixmap(cover.size()*2);
        if(!image.isNull()) {
            if(image.width()>image.height()*1.2){auto scaled=image.scaled(cover.size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);p->drawPixmap(cover,scaled,scaled.rect());}
            else icon.paint(p,QRect(cover.center()-QPoint(22,22),QSize(44,44)));
        }
        p->restore();p->setPen(text);p->setFont(option.font);
        const QStringList lines=index.data(Qt::UserRole+4).toString().split('\n');
        p->drawText(QRect(r.left()+12,cover.bottom()+10,r.width()-24,22),Qt::AlignLeft|Qt::AlignVCenter,option.fontMetrics.elidedText(lines.value(0),Qt::ElideRight,r.width()-24));
        if(lines.size()>1){QFont font=option.font;font.setPointSizeF(qMax(8.,font.pointSizeF()-1));p->setFont(font);p->setPen(dark?QColor("#aebfcd"):QColor("#536a7c"));p->drawText(QRect(r.left()+12,cover.bottom()+32,r.width()-24,18),Qt::AlignLeft|Qt::AlignVCenter,option.fontMetrics.elidedText(lines.value(1),Qt::ElideRight,r.width()-24));}
        if(option.state&QStyle::State_HasFocus){p->setBrush(Qt::NoBrush);p->setPen(QPen(QColor("#b7dcef"),2));p->drawRoundedRect(r.adjusted(1,1,-1,-1),14,14);}
        p->restore();
    }
};
}
#endif
