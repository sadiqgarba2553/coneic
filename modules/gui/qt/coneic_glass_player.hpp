/* Native floating glass transport. Copyright (C) 2026 Coneic contributors.
 * SPDX-License-Identifier: GPL-2.0-or-later. The VLC engine and output stay native. */
#ifndef CONEIC_GLASS_PLAYER_HPP
#define CONEIC_GLASS_PLAYER_HPP
#include "coneic_design.hpp"
#include "input_manager.hpp"
#include "actions_manager.hpp"
#include "dialogs_provider.hpp"
#include "menus.hpp"
#include "util/input_slider.hpp"
#include "adapters/seekpoints.hpp"
#include "components/controller.hpp"
#include <vlc_vout.h>
#include <vlc_image.h>
#include <vlc_picture.h>
#pragma push_macro("msleep")
#pragma push_macro("usleep")
#pragma push_macro("sleep")
#undef msleep
#undef usleep
#undef sleep
#include <QThread>
#pragma pop_macro("sleep")
#pragma pop_macro("usleep")
#pragma pop_macro("msleep")
#include <QTimer>
#include <QElapsedTimer>
#include <QPointer>
#include <QToolButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QCursor>
#include <QPropertyAnimation>
#include <QPainterPath>
#include <functional>

namespace ConeicPlayback {
inline QImage blur(QImage image)
{
    image=image.convertToFormat(QImage::Format_RGB32);
    const int w=image.width(),h=image.height(),radius=7,divisor=radius*2+1;
    if(w<2 || h<2)return image;
    QImage next(w,h,QImage::Format_RGB32);
    for(int pass=0;pass<3;pass++) {
        for(int axis=0;axis<2;axis++) {
            const int lines=axis?w:h,length=axis?h:w;
            for(int line=0;line<lines;line++) {
                auto at=[&](int offset)->QRgb {const int n=qBound(0,offset,length-1);return reinterpret_cast<const QRgb*>(image.constScanLine(axis?n:line))[axis?line:n];};
                int red=0,green=0,blue=0;
                for(int k=-radius;k<=radius;k++){const auto c=at(k);red+=qRed(c);green+=qGreen(c);blue+=qBlue(c);}
                for(int n=0;n<length;n++) {
                    reinterpret_cast<QRgb*>(next.scanLine(axis?n:line))[axis?line:n]=qRgb(red/divisor,green/divisor,blue/divisor);
                    const auto remove=at(n-radius),add=at(n+radius+1);
                    red+=qRed(add)-qRed(remove);green+=qGreen(add)-qGreen(remove);blue+=qBlue(add)-qBlue(remove);
                }
            }
            image.swap(next);
        }
    }
    return image;
}
class FrameWorker : public QThread {
    vout_thread_t *output;
public:
    QImage result;
    explicit FrameWorker(vout_thread_t *vout,QObject *parent):QThread(parent),output(vout){}
    ~FrameWorker() override {wait();if(output)vlc_object_release(output);}
    void run() override {
        picture_t *picture=nullptr;
        // Bound the wait; never block the Qt thread or encode/write screenshot files.
        if(vout_GetSnapshot(output,nullptr,&picture,nullptr,nullptr,CLOCK_FREQ/8)==VLC_SUCCESS && picture) {
            image_handler_t *converter=image_HandlerCreate(output);
            if(converter) {
                video_format_t format;video_format_Init(&format,VLC_CODEC_RGB32);
                const auto &source=picture->format;
                const int sourceWidth=qMax(1,int(source.i_visible_width)),sourceHeight=qMax(1,int(source.i_visible_height));
                format.i_width=format.i_visible_width=320;
                format.i_height=format.i_visible_height=qMax(1,320*sourceHeight/sourceWidth);
                format.i_sar_num=format.i_sar_den=1;
                format.i_rmask=0x00ff0000;format.i_gmask=0x0000ff00;format.i_bmask=0x000000ff;
                picture_t *converted=image_Convert(converter,picture,&source,&format);
                if(converted) {
                    QImage sample(converted->p[0].p_pixels,format.i_visible_width,format.i_visible_height,converted->p[0].i_pitch,QImage::Format_RGB32);
                    result=blur(sample.copy());picture_Release(converted);
                }
                video_format_Clean(&format);image_HandlerDelete(converter);
            }
            picture_Release(picture);
        }
        vlc_object_release(output);output=nullptr;
    }
};
class GlyphButton : public QToolButton {
    QString glyph;
public:
    bool primary=false;
    explicit GlyphButton(const QString &symbol,const QString &label,QWidget *parent):QToolButton(parent),glyph(symbol) {
        setToolTip(label);setAccessibleName(label);setFocusPolicy(Qt::TabFocus);setCursor(Qt::PointingHandCursor);setFixedSize(36,36);
    }
    void setGlyph(const QString &symbol){glyph=symbol;update();}
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const bool contrast=ConeicDesign::highContrast();
        QColor ink=contrast?palette().color(QPalette::WindowText):QColor("#f4f8fc");
        if(!isEnabled())ink.setAlpha(85);
        if(width()<=50 && (primary || underMouse() || isDown())) {p.setPen(Qt::NoPen);p.setBrush(QColor(255,255,255,isDown()?42:underMouse()?28:18));p.drawRoundedRect(rect().adjusted(1,1,-1,-1),primary?20:10,primary?20:10);}
        if(hasFocus()){p.setBrush(Qt::NoBrush);p.setPen(QPen(ink,1));if(width()>50)p.drawEllipse(rect().adjusted(2,2,-3,-3));else p.drawRoundedRect(rect().adjusted(2,2,-3,-3),9,9);}
        p.translate(width()/2.,height()/2.);if(width()>50)p.scale(1.8,1.8);p.setPen(QPen(ink,1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);
        if(glyph=="play") {QPainterPath path;path.moveTo(-3,-7);path.lineTo(7,0);path.lineTo(-3,7);path.closeSubpath();p.fillPath(path,ink);}
        else if(glyph=="pause"){p.setPen(Qt::NoPen);p.setBrush(ink);p.drawRoundedRect(QRectF(-6,-7,4,14),1,1);p.drawRoundedRect(QRectF(2,-7,4,14),1,1);}
        else if(glyph=="back" || glyph=="forward") {
            if(glyph=="forward")p.scale(-1,1);
            p.drawArc(QRectF(-8,-8,16,16),55*16,285*16);p.drawLine(QPointF(-7,-8),QPointF(-7,-3));p.drawLine(QPointF(-7,-3),QPointF(-2,-3));
            if(glyph=="forward")p.scale(-1,1);
            QFont font=this->font();font.setPixelSize(8);font.setWeight(QFont::DemiBold);p.setFont(font);p.drawText(QRectF(-7,-6,14,14),Qt::AlignCenter,"10");
        }
        else if(glyph=="subtitles"){p.drawRoundedRect(QRectF(-9,-7,18,14),3,3);p.drawLine(QPointF(-5,2),QPointF(-2,2));p.drawLine(QPointF(1,2),QPointF(5,2));p.drawLine(QPointF(-5,-2),QPointF(5,-2));}
        else if(glyph=="fullscreen"){for(int i=0;i<4;i++){p.rotate(90);p.drawLine(QPointF(-8,-3),QPointF(-8,-8));p.drawLine(QPointF(-8,-8),QPointF(-3,-8));}}
        else if(glyph=="more"){p.setPen(Qt::NoPen);p.setBrush(ink);for(int y=-6;y<=6;y+=6)p.drawEllipse(QPointF(0,y),1.5,1.5);}
        else if(glyph.startsWith("volume")) {
            QPainterPath path;path.moveTo(-9,-3);path.lineTo(-5,-3);path.lineTo(0,-7);path.lineTo(0,7);path.lineTo(-5,3);path.lineTo(-9,3);path.closeSubpath();p.drawPath(path);
            if(glyph=="volume-muted"){p.drawLine(QPointF(4,-3),QPointF(9,3));p.drawLine(QPointF(4,3),QPointF(9,-3));}
            else{p.drawArc(QRectF(-1,-5,10,10),-55*16,110*16);p.drawArc(QRectF(-4,-8,16,16),-50*16,100*16);}
        }
        else if(glyph=="library"){p.drawRoundedRect(QRectF(-8,-7,16,14),3,3);p.drawLine(QPointF(-3,-7),QPointF(-3,7));}
    }
};
class GlassSurface : public QWidget {
public:
    QImage backdrop;
    QRect scene;
    bool circular=false;
    explicit GlassSurface(QWidget *parent):QWidget(parent){}
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        QRectF r=rect().adjusted(1,1,-1,-1);QPainterPath shape;
        if(circular)shape.addEllipse(r);else shape.addRoundedRect(r,19,19);
        p.setClipPath(shape);
        const bool contrast=ConeicDesign::highContrast(),transparency=qApp->property("coneicTransparency").toBool()&&!contrast;
        p.fillRect(rect(),contrast?palette().color(QPalette::Window):QColor("#172a39"));
        if(transparency && !backdrop.isNull() && scene.isValid()) {
            QSizeF image=backdrop.size();image.scale(scene.size(),Qt::KeepAspectRatio);
            QRectF target(QPointF(scene.center())-QPointF(image.width()/2,image.height()/2)-QPointF(mapToGlobal(QPoint())),image);
            p.drawImage(target,backdrop);p.fillRect(rect(),QColor(8,26,40,160));
        }
        if(transparency){QLinearGradient sheen(r.topLeft(),r.bottomRight());sheen.setColorAt(0,QColor(225,244,255,20));sheen.setColorAt(.5,QColor(210,240,255,4));sheen.setColorAt(1,QColor(3,15,25,28));p.fillPath(shape,sheen);}
        p.setClipping(false);p.setBrush(Qt::NoBrush);p.setPen(QPen(contrast?palette().color(QPalette::WindowText):QColor(229,245,255,55),1));p.drawPath(shape);
    }
};
}

class ConeicGlassPlayer : public QObject {
    intf_thread_t *intf;
    QMainWindow *main;
    QPointer<QWidget> video,legacyControls,legacySeek;
    QVBoxLayout *mainLayout;
    ConeicPlayback::GlassSurface *bar,*center;
    ConeicPlayback::GlyphButton *play,*centerPlay,*sound;
    QLabel *title,*subtitle,*time;
    SeekSlider *seek;
    QSlider *volume;
    AdvControlsWidget *advanced;
    QTimer *poll;
    QElapsedTimer clock;
    QPropertyAnimation *fade;
    QPointer<ConeicPlayback::FrameWorker> worker;
    QPoint pointer;
    qint64 lastInteraction=0,lastSample=-1000,feedbackUntil=0;
    int generation=0;
    bool floating=false,shown=true,keyboardFocus=false;
    QString currentTitle;
    std::function<void()> library;
    static QString stamp(int seconds) {
        seconds=qMax(0,seconds);return seconds>=3600?QString("%1:%2:%3").arg(seconds/3600).arg(seconds/60%60,2,10,QChar('0')).arg(seconds%60,2,10,QChar('0')):QString("%1:%2").arg(seconds/60,2,10,QChar('0')).arg(seconds%60,2,10,QChar('0'));
    }
    void reveal(bool immediate=false) {
        lastInteraction=clock.elapsed();
        if(main->property("coneicMinimalView").toBool()){fade->stop();bar->hide();center->hide();return;}
        if(!bar->isVisible() && floating){bar->show();bar->raise();}
        if(shown && !immediate)return;shown=true;
        fade->stop();
        if(immediate || !floating || !ConeicDesign::motionEnabled()){bar->setWindowOpacity(1);return;}
        fade->setStartValue(bar->windowOpacity());fade->setEndValue(1.);fade->setDuration(150);fade->start();
    }
    void hideChrome() {
        if(!shown)return;shown=false;fade->stop();
        if(!ConeicDesign::motionEnabled()){bar->setWindowOpacity(0);bar->hide();return;}
        fade->setStartValue(bar->windowOpacity());fade->setEndValue(0.);fade->setDuration(125);fade->start();
    }
    void jump(int seconds) {
        input_thread_t *input=MainInputManager::getInstance(intf)->getInput();
        if(input && var_GetBool(input,"can-seek"))var_SetInteger(input,"time",qMax<int64_t>(0,var_GetInteger(input,"time")+int64_t(seconds)*CLOCK_FREQ));
        reveal(true);
    }
    void sample() {
        if(worker || !floating || !bar->isVisible() || !qApp->property("coneicTransparency").toBool() || clock.elapsed()-lastSample<500)return;
        auto manager=MainInputManager::getInstance(intf);
        if(manager->getIM()->playingStatus()!=PLAYING_S)return;
        auto output=manager->getVout();if(!output)return;
        lastSample=clock.elapsed();const int epoch=generation;
        worker=new ConeicPlayback::FrameWorker(output,this);auto job=worker.data();
        connect(job,&QThread::finished,this,[this,job,epoch](){
            if(epoch==generation && !job->result.isNull()) {
                bar->backdrop=center->backdrop=job->result;
                bar->setProperty("coneicLiveBackdrop",true);bar->update();center->update();
            }
            worker=nullptr;job->deleteLater();
        });job->start();
    }
    void sync() {
        if(legacyControls)legacyControls->hide();if(legacySeek)legacySeek->hide();
        auto manager=MainInputManager::getInstance(intf);const bool hasInput=manager->getIM()->hasInput();
        const bool videoMode=hasInput && video && video->isVisible() && manager->getIM()->hasVideo();
        if(videoMode!=floating) {
            fade->stop();bar->hide();mainLayout->removeWidget(bar);floating=videoMode;
            if(floating){bar->setParent(main,Qt::Tool|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);bar->setAttribute(Qt::WA_TranslucentBackground);bar->setAttribute(Qt::WA_ShowWithoutActivating);}
            else{bar->setParent(main->centralWidget(),Qt::Widget);mainLayout->addWidget(bar);bar->setWindowOpacity(1);}
            bar->setWindowOpacity(1);main->menuBar()->setVisible(!floating && !main->property("coneicMinimalView").toBool());shown=true;lastInteraction=clock.elapsed();
        }
        const bool allowed=hasInput && !main->property("coneicMinimalView").toBool() && main->isVisible() && !main->isMinimized() && !QApplication::activeModalWidget() && (QGuiApplication::applicationState()==Qt::ApplicationActive || !floating);
        if(floating && main->menuBar()->isVisible())main->menuBar()->hide();
        if(!allowed){bar->hide();center->hide();return;}
        const QPoint cursor=QCursor::pos();
        if(cursor!=pointer){pointer=cursor;if(main->frameGeometry().contains(cursor))reveal();}
        const int panelHeight=advanced->isHidden()?128:172;
        if(!floating){bar->show();bar->setFixedHeight(panelHeight);center->hide();return;}
        const QRect scene(video->mapToGlobal(QPoint()),video->size());bar->scene=center->scene=scene;
        const int width=qMin(1020,qMax(280,scene.width()-48));
        const QRect geometry(scene.center().x()-width/2,scene.bottom()-panelHeight-16,width,panelHeight);
        if(bar->geometry()!=geometry)bar->setGeometry(geometry);
        volume->setVisible(width>=600);subtitle->setVisible(width>=430);
        const bool paused=manager->getIM()->playingStatus()!=PLAYING_S;
        const bool menuOpen=QApplication::activePopupWidget()!=nullptr;
        const bool focused=keyboardFocus && bar->isAncestorOf(QApplication::focusWidget());
        if(paused || menuOpen || focused || bar->geometry().contains(cursor))reveal(true);
        else if(clock.elapsed()-lastInteraction>2800)hideChrome();
        if(shown && !bar->isVisible()){bar->show();bar->raise();}
        if(paused || clock.elapsed()<feedbackUntil){
            center->setGeometry(QRect(scene.center()-QPoint(43,43),QSize(86,86)));center->show();center->raise();
        }else center->hide();
        sample();
    }
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()==QEvent::KeyPress && main->isVisible()) {
            auto widget=qobject_cast<QWidget*>(object);
            if(widget && (widget==main || main->isAncestorOf(widget) || widget==bar || bar->isAncestorOf(widget))){keyboardFocus=true;reveal(true);}
        }else if(event->type()==QEvent::MouseButtonPress){keyboardFocus=false;}
        else if(event->type()==QEvent::MouseMove){auto widget=qobject_cast<QWidget*>(object);if(widget && (main->isAncestorOf(widget) || bar->isAncestorOf(widget)))reveal();}
        return false;
    }
public:
    ConeicGlassPlayer(intf_thread_t *p,QMainWindow *window,QWidget *viewport,QVBoxLayout *layout,std::function<void()> openLibrary):QObject(window),intf(p),main(window),video(viewport),mainLayout(layout),library(openLibrary) {
        setObjectName("ConeicGlassPlayer");qApp->setProperty("coneicGlassPlayback",true);clock.start();pointer=QCursor::pos();
        bar=new ConeicPlayback::GlassSurface(main->centralWidget());bar->setObjectName("ConeicGlassDock");bar->setAttribute(Qt::WA_StyledBackground,false);bar->setMinimumWidth(280);
        auto content=new QVBoxLayout(bar);content->setContentsMargins(22,12,22,12);content->setSpacing(3);
        auto heading=new QHBoxLayout;title=new QLabel(qtr("Now playing"),bar);title->setTextFormat(Qt::PlainText);title->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);title->setStyleSheet("font-family: Inter; font-weight: 500; font-size: 14px;");heading->addWidget(title,1);
        time=new QLabel("0:00 / 0:00",bar);time->setAccessibleName(qtr("Playback time"));time->setStyleSheet(ConeicDesign::highContrast()?"font-size: 11px;":"font-size: 11px; color: #c8dce9;");heading->addWidget(time);content->addLayout(heading);
        subtitle=new QLabel(qtr("Now playing"),bar);subtitle->setStyleSheet(ConeicDesign::highContrast()?"font-size: 11px;":"font-size: 11px; color: #b5cbd9;");content->addWidget(subtitle);
        seek=new SeekSlider(intf,Qt::Horizontal,bar,true);seek->setObjectName("ConeicGlassSeek");seek->setAccessibleName(qtr("Playback position"));seek->setFixedHeight(17);content->addWidget(seek);
        auto manager=MainInputManager::getInstance(intf);auto input=manager->getIM();
        connect(input,&InputManager::positionUpdated,seek,&SeekSlider::setPosition);connect(seek,&SeekSlider::sliderDragged,input,&InputManager::sliderUpdate);connect(input,&InputManager::inputCanSeek,seek,&SeekSlider::setSeekable);connect(input,&InputManager::cachingChanged,seek,&SeekSlider::updateBuffering);
        auto chapters=new SeekPoints(this,intf);seek->setChapters(chapters);connect(input,&InputManager::chapterChanged,chapters,&SeekPoints::update);
        connect(input,&InputManager::positionUpdated,this,[this](float,int64_t elapsed,int duration){time->setText(stamp(int(elapsed/CLOCK_FREQ))+" / "+stamp(duration));});
        auto buttons=new QHBoxLayout;buttons->setSpacing(5);
        play=new ConeicPlayback::GlyphButton("play",qtr("Play or pause"),bar);play->setObjectName("ConeicGlassPlay");play->primary=true;play->setFixedSize(40,40);buttons->addWidget(play);connect(play,&QToolButton::clicked,manager,&MainInputManager::togglePlayPause);
        auto back=new ConeicPlayback::GlyphButton("back",qtr("Back 10 seconds"),bar);back->setObjectName("ConeicGlassBack");buttons->addWidget(back);connect(back,&QToolButton::clicked,this,[this](){jump(-10);});
        auto forward=new ConeicPlayback::GlyphButton("forward",qtr("Forward 10 seconds"),bar);forward->setObjectName("ConeicGlassForward");buttons->addWidget(forward);connect(forward,&QToolButton::clicked,this,[this](){jump(10);});
        auto home=new ConeicPlayback::GlyphButton("library",qtr("Open library"),bar);buttons->addWidget(home);connect(home,&QToolButton::clicked,this,[this](){library();sync();});buttons->addStretch();
        sound=new ConeicPlayback::GlyphButton("volume",qtr("Mute or unmute"),bar);buttons->addWidget(sound);connect(sound,&QToolButton::clicked,ActionsManager::getInstance(intf),&ActionsManager::toggleMuteAudio);
        connect(manager,&MainInputManager::soundMuteChanged,this,[this](bool muted){sound->setGlyph(muted?"volume-muted":"volume");});
        volume=new QSlider(Qt::Horizontal,bar);volume->setObjectName("ConeicGlassVolume");volume->setAccessibleName(qtr("Volume"));volume->setRange(0,125);volume->setFixedWidth(90);volume->setValue(qRound(playlist_VolumeGet(pl_Get(intf))*100));buttons->addWidget(volume);
        connect(volume,&QSlider::valueChanged,this,[this](int value){playlist_VolumeSet(pl_Get(intf),value/100.f);});
        connect(manager,&MainInputManager::volumeChanged,this,[this](float value){const bool blocked=volume->blockSignals(true);volume->setValue(qRound(value*100));volume->blockSignals(blocked);});
        auto captions=new ConeicPlayback::GlyphButton("subtitles",qtr("Subtitles"),bar);captions->setObjectName("ConeicGlassSubtitles");buttons->addWidget(captions);
        for(auto action:main->menuBar()->actions())if(action->text()==qtr("Subti&tle") && action->menu()){captions->setMenu(action->menu());break;}
        captions->setPopupMode(QToolButton::InstantPopup);
        auto fullscreen=new ConeicPlayback::GlyphButton("fullscreen",qtr("Toggle fullscreen"),bar);fullscreen->setObjectName("ConeicGlassFullscreen");buttons->addWidget(fullscreen);connect(fullscreen,&QToolButton::clicked,this,[this](){ActionsManager::getInstance(intf)->doAction(FULLSCREEN_ACTION);reveal(true);});
        auto more=new ConeicPlayback::GlyphButton("more",qtr("More playback options"),bar);more->setObjectName("ConeicGlassMore");buttons->addWidget(more);auto menu=new QMenu(more);more->setMenu(menu);more->setPopupMode(QToolButton::InstantPopup);
        connect(menu,&QMenu::aboutToShow,this,[this,menu](){menu->clear();auto open=menu->addAction(qtr("Open media"));connect(open,&QAction::triggered,DialogsProvider::getInstance(intf),&DialogsProvider::simpleOpenDialog);auto settings=menu->addAction(qtr("Settings"));connect(settings,&QAction::triggered,DialogsProvider::getInstance(intf),&DialogsProvider::prefsDialog);menu->addSeparator();menu->addActions(main->menuBar()->actions());});
        content->addLayout(buttons);advanced=new AdvControlsWidget(intf,bar);advanced->setVisible(intf->p_sys->mainSettings->value("MainWindow/adv-controls",false).toBool());content->addWidget(advanced);mainLayout->addWidget(bar);bar->hide();
        center=new ConeicPlayback::GlassSurface(main);center->circular=true;center->setObjectName("ConeicGlassFeedback");center->setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);center->setAttribute(Qt::WA_TranslucentBackground);center->setAttribute(Qt::WA_ShowWithoutActivating);
        auto centerLayout=new QVBoxLayout(center);centerLayout->setContentsMargins(13,13,13,13);centerPlay=new ConeicPlayback::GlyphButton("play",qtr("Resume playback"),center);centerPlay->setStyleSheet("QToolButton, QToolButton:hover, QToolButton:focus { background: transparent; border: 0; }");centerPlay->setFixedSize(60,60);centerLayout->addWidget(centerPlay);connect(centerPlay,&QToolButton::clicked,manager,&MainInputManager::togglePlayPause);center->hide();
        fade=new QPropertyAnimation(bar,"windowOpacity",this);QEasingCurve ease(QEasingCurve::BezierSpline);ease.addCubicBezierSegment(QPointF(.23,1),QPointF(.32,1),QPointF(1,1));fade->setEasingCurve(ease);connect(fade,&QPropertyAnimation::finished,this,[this](){if(!shown)bar->hide();});
        connect(input,&InputManager::playingStatusChanged,this,[this](int status){play->setGlyph(status==PLAYING_S?"pause":"play");centerPlay->setGlyph(status==PLAYING_S?"pause":"play");feedbackUntil=clock.elapsed()+450;reveal(true);sync();});
        connect(input,&InputManager::nameChanged,this,[this](const QString &name){currentTitle=name;title->setText(name);bar->backdrop=QImage();center->backdrop=QImage();generation++;subtitle->setText(qtr("Now playing"));});
        connect(manager,&MainInputManager::inputChanged,this,[this](bool){generation++;reveal(true);sync();});
        qApp->installEventFilter(this);poll=new QTimer(this);poll->setInterval(100);connect(poll,&QTimer::timeout,this,[this](){sync();});poll->start();
    }
    void setLegacy(QWidget *controls,QWidget *input){legacyControls=controls;legacySeek=input;if(auto toolbar=qobject_cast<ControlsWidget*>(controls))connect(toolbar,&ControlsWidget::advancedControlsToggled,this,[this](bool visible){advanced->setVisible(visible);sync();});sync();}
    ~ConeicGlassPlayer() override {poll->stop();if(worker){worker->wait();delete worker.data();}delete bar;delete center;qApp->setProperty("coneicGlassPlayback",false);}
};
#endif
