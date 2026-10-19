/* Opt-in acceptance checks for the real native glass player.
 * SPDX-License-Identifier: GPL-2.0-or-later. Never enabled in ordinary launches. */
#ifndef CONEIC_GLASS_SMOKE_HPP
#define CONEIC_GLASS_SMOKE_HPP
#include <QScreen>
#include <QWindow>
#include <QToolButton>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QFile>
#include <memory>
#include <functional>
namespace ConeicGlassSmoke {
inline void capture(QWidget *main,const QString &path) {
    if(!main)return;
#ifdef Q_OS_WIN
    SetWindowPos((HWND)main->winId(),HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    for(auto widget:qApp->topLevelWidgets())if(widget!=main && widget->isVisible() && widget->parentWidget()==main)widget->raise();
    QCoreApplication::processEvents();Sleep(120);
#endif
    auto screen=main->windowHandle()?main->windowHandle()->screen():QGuiApplication::primaryScreen();
    if(!screen)return;
    const QRect r=main->frameGeometry();screen->grabWindow(0,r.x(),r.y(),r.width(),r.height()).save(path);
#ifdef Q_OS_WIN
    SetWindowPos((HWND)main->winId(),HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif
}
inline void run(intf_thread_t *intf,const QString &directory,std::function<void(bool)> complete)
{
    QWidget *main=nullptr;for(auto widget:qApp->topLevelWidgets())if(widget->inherits("QMainWindow")){main=widget;break;}
    auto checks=std::make_shared<QJsonArray>();
    auto check=[checks](const QString &name,bool pass){checks->append(QJsonObject{{"name",name},{"passed",pass}});};
    if(!main){complete(false);return;}
    auto bar=main->findChild<QWidget*>("ConeicGlassDock");
    auto play=main->findChild<QToolButton*>("ConeicGlassPlay");
    auto forward=main->findChild<QToolButton*>("ConeicGlassForward");
    auto fullscreen=main->findChild<QToolButton*>("ConeicGlassFullscreen");
    auto manager=MainInputManager::getInstance(intf);
    auto audio=main->findChild<QToolButton*>("ConeicGlassAudio");auto speed=main->findChild<QToolButton*>("ConeicGlassSpeed");
    check("Audio tracks remain directly available in the floating controls",audio&&audio->menu());
    check("Playback speed has a keyboard-accessible menu",speed&&speed->menu());
    if(speed&&speed->menu()){
        QMetaObject::invokeMethod(speed->menu(),"aboutToShow",Qt::DirectConnection);
        for(auto action:speed->menu()->actions())if(action->text()=="1.5x")action->trigger();
        auto input=manager->getInput();check("Speed selection changes the native playback rate",input&&qAbs(var_GetFloat(input,"rate")-1.5)<.01);
        for(auto action:speed->menu()->actions())if(action->text()=="1x")action->trigger();
    }
    check("Glass controls float in a separate native window",bar&&bar->isWindow()&&bar->isVisible());
    check("Playback material uses live blur or the requested solid fallback",bar && (qApp->property("coneicTransparency").toBool()?bar->property("coneicLiveBackdrop").toBool():!bar->property("coneicLiveBackdrop").toBool()));
    check("Seek, subtitle and more controls remain accessible",main->findChild<QWidget*>("ConeicGlassSeek")&&main->findChild<QWidget*>("ConeicGlassSubtitles")&&main->findChild<QWidget*>("ConeicGlassMore"));
    if(bar)bar->grab().save(directory+"/glass-controls.png");capture(main,directory+"/glass-player-desktop.png");
    if(!play||!forward||!fullscreen){complete(false);return;}
    play->click();
    QTimer::singleShot(220,qApp,[=](){
        check("Floating play control pauses the native input",manager->getIM()->playingStatus()==PAUSE_S);
        check("Paused controls remain visible",bar&&bar->isVisible());
        capture(main,directory+"/glass-player-paused.png");
        auto input=manager->getInput();const int64_t before=input?var_GetInteger(input,"time"):0;
        forward->click();
        QTimer::singleShot(220,qApp,[=](){
            auto input=manager->getInput();const int64_t after=input?var_GetInteger(input,"time"):0;
            check("10-second control seeks the native input",after>=before+8*CLOCK_FREQ);
            QKeyEvent space(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);QCoreApplication::sendEvent(bar,&space);fullscreen->click();
            QTimer::singleShot(350,qApp,[=](){
                check("Fullscreen retains the floating glass controls",main->isFullScreen()&&bar&&bar->isVisible());
                capture(main,directory+"/glass-player-fullscreen.png");
                check("Space resumes playback from the glass surface",manager->getIM()->playingStatus()==PLAYING_S);
                QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QCoreApplication::sendEvent(bar,&escape);
                QTimer::singleShot(800,qApp,[=](){
                    check("Escape leaves fullscreen while glass controls are focused",!main->isFullScreen());
                    QMetaObject::invokeMethod(main,"toggleMinimalView",Qt::DirectConnection,Q_ARG(bool,true));
                    QTimer::singleShot(180,qApp,[=](){
                    check("Native minimal-view action hides the floating transport",main->property("coneicMinimalView").toBool() && !bar->isVisible());
                    checks->append(QJsonObject{{"name","Minimal-view transition diagnostic"},{"passed",true},{"fullscreen",main->isFullScreen()},{"minimal",main->property("coneicMinimalView").toBool()},{"transportVisible",bar->isVisible()}});
                    QMetaObject::invokeMethod(main,"toggleMinimalView",Qt::DirectConnection,Q_ARG(bool,false));
                    bool passed=true;for(auto v:*checks)passed &= v.toObject()["passed"].toBool();
                    QFile report(directory+"/glass-player-results.json");if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(QJsonObject{{"passed",passed},{"checks",*checks}}).toJson());
                    complete(passed);
                    });
                });
            });
        });
    });
}
}
#endif
