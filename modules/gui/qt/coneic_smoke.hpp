/*****************************************************************************
 * coneic_smoke.hpp: opt-in native interface acceptance checks
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Disabled unless CONEIC_TEST_OUTPUT is explicitly set by the test runner.
 *****************************************************************************/
#ifndef CONEIC_SMOKE_HPP
#define CONEIC_SMOKE_HPP
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>
#include <QListWidget>
#include <QLineEdit>
#include <QTabWidget>
#include <QCheckBox>
#include <QMouseEvent>
#include <QPointer>
#include <QFileInfo>
#include "recents.hpp"
#include <memory>
#include <functional>
#include "dialogs_provider.hpp"
#include "coneic_settings.hpp"

namespace ConeicSmoke {
inline void schedule(QApplication &app, intf_thread_t *intf)
{
    const QString directory = QString::fromLocal8Bit(qgetenv("CONEIC_TEST_OUTPUT"));
    if (directory.isEmpty())
        return;
    QDir().mkpath(directory);
    struct State {
        QJsonArray results;
        int index = 0;
        bool awaiting = false;
        bool allPassed = true;
        std::function<void()> next;
    };
    auto state = std::make_shared<State>();
    QTimer::singleShot(1200, &app, [state, directory, intf, &app]() {
        QWidget *main = nullptr;
        for (QWidget *widget : app.topLevelWidgets())
            if (widget->inherits("QMainWindow") && widget->isVisible()) { main = widget; break; }
        bool mainOk = main && main->windowTitle().contains(QStringLiteral("Coneic"));
        state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Branded native main window")},{QStringLiteral("passed"),mainOk}});
        state->allPassed &= mainOk;
        if (main) {
            main->resize(1000,680);
            main->grab().save(directory + QStringLiteral("/01-main-window.png"));
            QListWidget *library=main->findChild<QListWidget*>(QStringLiteral("ConeicLibraryNavigation"));
            bool libraryPages=library && library->count()==6;
            if(library)for(int page=0;page<library->count();page++) {
                library->setCurrentRow(page);app.processEvents();
                main->grab().save(directory+QStringLiteral("/library-%1.png").arg(page,2,10,QChar('0')));
            }
            if(library)library->setCurrentRow(0);
            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Library navigation pages render")},{QStringLiteral("passed"),libraryPages}});
            state->allPassed &= libraryPages;
            QJsonArray menuNames;
            for (QMenuBar *bar : main->findChildren<QMenuBar*>())
                for (QAction *action : bar->actions()) menuNames.append(action->text());
            QJsonArray actions;
            for (QAction *action : main->findChildren<QAction*>())
                if (!action->text().isEmpty()) actions.append(action->text());
            QJsonObject inventory{{QStringLiteral("menus"),menuNames},{QStringLiteral("actions"),actions},
                {QStringLiteral("fontFamily"),app.font().family()},{QStringLiteral("fontPoints"),app.font().pointSizeF()},
                {QStringLiteral("stylesheetApplied"),!app.styleSheet().isEmpty()}};
            QFile file(directory + QStringLiteral("/ui-inventory.json"));
            if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(inventory).toJson());
        }
        const QList<QPair<QByteArray,QString>> cases = {
            {QByteArray("prefsDialog"),QStringLiteral("Preferences")},
            {QByteArray("extendedDialog"),QStringLiteral("Effects and filters")},
            {QByteArray("openDiscDialog"),QStringLiteral("Disc workflow")},
            {QByteArray("openNetDialog"),QStringLiteral("Network workflow")},
            {QByteArray("openCaptureDialog"),QStringLiteral("Capture workflow")},
            {QByteArray("bookmarksDialog"),QStringLiteral("Bookmarks")},
            {QByteArray("vlmDialog"),QStringLiteral("VLM broadcast manager")},
            {QByteArray("openAndStreamingDialogs"),QStringLiteral("Stream entry dialog")},
            {QByteArray("openAndTranscodingDialogs"),QStringLiteral("Conversion entry dialog")}
        };
        auto timer = new QTimer(&app);
        timer->setInterval(650);
        QObject::connect(timer, &QTimer::timeout, &app, [state, directory, intf, &app, timer, cases]() {
            if (state->index >= cases.size()) {
                timer->stop();
                for(QWidget *widget:app.topLevelWidgets())if(widget->inherits("QMainWindow") && widget->isVisible())widget->grab().save(directory+QStringLiteral("/library-with-thumbnails.png"));
                auto complete=[state,directory,intf,&app,timer](){
                for(QWidget *widget:app.topLevelWidgets())if(widget->inherits("QMainWindow") && widget->isVisible())widget->grab().save(directory+QStringLiteral("/01-main-window-final.png"));
                QJsonObject result{{QStringLiteral("passed"),state->allPassed},
                    {QStringLiteral("checks"),state->results},
                    {QStringLiteral("scope"),QStringLiteral("Native UI opening, rendering, branding and menu inventory. Devices, network transmission and protected discs are not exercised.")}};
                QFile file(directory + QStringLiteral("/native-ui-results.json"));
                if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(result).toJson()); file.close(); }
                DialogsProvider::getInstance(intf)->quit();
                timer->deleteLater();
                };
                const QString media=QString::fromLocal8Bit(qgetenv("CONEIC_TEST_MEDIA"))+QStringLiteral("/Coneic motion.avi");
                if(QFileInfo::exists(media)) {
                    Open::openMRL(intf,QString::fromUtf8(QUrl::fromLocalFile(media).toEncoded()));
                    QTimer::singleShot(1800,&app,[state,directory,intf,&app,complete](){
                        QWidget *main=nullptr,*slider=nullptr;
                        for(QWidget *widget:app.topLevelWidgets())if(widget->inherits("QMainWindow") && widget->isVisible()){main=widget;break;}
                        if(main)for(QWidget *widget:main->findChildren<QWidget*>())if(widget->inherits("SeekSlider") && widget->isVisible()){slider=widget;break;}
                        const bool playing=MainInputManager::getInstance(intf)->getIM()->playingStatus()==PLAYING_S && slider && slider->isEnabled();
                        state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Native player playback and seek control active")},{QStringLiteral("passed"),playing}});state->allPassed &= playing;
                        if(main)main->grab().save(directory+QStringLiteral("/native-player.png"));
                        if(slider) {
                            QEvent enter(QEvent::Enter);QApplication::sendEvent(slider,&enter);
                            const QPoint local(slider->width()/3,slider->height()/2);
                            QMouseEvent move(QEvent::MouseMove,QPointF(local),QPointF(slider->mapToGlobal(local)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
                            QApplication::sendEvent(slider,&move);
                        }
                        auto poll=new QTimer(&app);poll->setInterval(200);
                        auto attempts=std::make_shared<int>(0);
                        QObject::connect(poll,&QTimer::timeout,&app,[state,directory,&app,poll,attempts,complete](){
                            bool preview=false;
                            for(QWidget *widget:app.topLevelWidgets())if(widget->inherits("TimeTooltip") && widget->property("coneicHasPreview").toBool()) {
                                preview=true;widget->grab().save(directory+QStringLiteral("/native-seek-preview.png"));
                            }
                            if(!preview && ++*attempts<40)return;
                            poll->stop();poll->deleteLater();
                            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Seek hover renders a real native VLC video thumbnail")},{QStringLiteral("passed"),preview}});state->allPassed &= preview;
                            const int thumbnails=app.property("coneicThumbnailCompleted").toInt()+app.property("coneicThumbnailCacheHits").toInt();
                            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Background video thumbnails generated or loaded")},{QStringLiteral("count"),thumbnails},{QStringLiteral("passed"),thumbnails>0}});state->allPassed &= thumbnails>0;
                            complete();
                        });poll->start();
                    });
                } else complete();
                return;
            }
            const auto current = cases.at(state->index);
            if (!state->awaiting) {
                bool invoked = QMetaObject::invokeMethod(DialogsProvider::getInstance(intf), current.first.constData(), Qt::QueuedConnection);
                state->awaiting = true;
                if (!invoked) state->allPassed = false;
            } else {
                bool visible = false;
                for (QWidget *widget : app.topLevelWidgets()) {
                    if (widget->isVisible() && widget->isWindow() && !widget->inherits("QMainWindow") && !widget->windowTitle().isEmpty()) {
                        visible = true;
                        if (current.first=="prefsDialog") {
                            timer->stop();
                            QWidget *settings=widget->findChild<QWidget*>(QStringLiteral("ConeicGlobalSettings"));
                            QListWidget *navigation=widget->findChild<QListWidget*>(QStringLiteral("ConeicSettingsNavigation"));
                            bool categories=settings && navigation && navigation->count()==13;
                            if(navigation)for(int page=0;page<navigation->count();page++) {
                                navigation->setCurrentRow(page);
                                app.processEvents();
                                widget->grab().save(directory+QStringLiteral("/settings-%1.png").arg(page,2,10,QChar('0')));
                            }
                            if(navigation)navigation->setCurrentRow(0);
                            QToolButton *light=nullptr,*dark=nullptr;
                            if(settings)for(QToolButton *button:settings->findChildren<QToolButton*>()) {
                                if(button->text()==qtr("Light"))light=button;
                                if(button->text()==qtr("Dark"))dark=button;
                            }
                            bool themes=light&&dark;
                            if(themes) {
                                light->click();app.processEvents();
                                themes=app.palette().color(QPalette::Window).lightness()>128;
                                widget->grab().save(directory+QStringLiteral("/settings-light.png"));
                                dark->click();app.processEvents();
                                themes=themes && app.palette().color(QPalette::Window).lightness()<128;
                            }
                            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Immediate light and dark theme selection")},{QStringLiteral("passed"),themes}});
                            state->allPassed &= themes;
                            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("All 13 global settings categories render")},{QStringLiteral("passed"),categories}});
                            state->allPassed &= categories;
                            const int optionCount=settings?settings->property("coneicOptionCount").toInt():0;
                            bool inventory=optionCount>500;
                            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Runtime settings inventory")},{QStringLiteral("count"),optionCount},{QStringLiteral("passed"),inventory}});
                            state->allPassed &= inventory;
                            if(settings) {
                                QLineEdit *search=settings->findChild<QLineEdit*>();
                                if(search) {
                                    search->setText(QStringLiteral("avcodec"));app.processEvents();
                                    widget->grab().save(directory+QStringLiteral("/settings-search.png"));
                                    bool matches=false;
                                    for(QWidget *row:settings->findChildren<QWidget*>()) if(row->isVisible() && row->property("coneicOption").toString().contains("avcodec"))matches=true;
                                    state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Search finds advanced decoder options")},{QStringLiteral("passed"),matches}});
                                    state->allPassed &= matches;
                                    search->clear();
                                    search->setText(QStringLiteral("qt-video-autoresize"));app.processEvents();
                                    QWidget *target=nullptr;
                                    for(QWidget *row:settings->findChildren<QWidget*>())if(row->isVisible() && row->property("coneicOption").toString()=="qt-video-autoresize")target=row;
                                    QCheckBox *toggle=target?target->findChild<QCheckBox*>():nullptr;
                                    const int original=config_GetInt(intf,"qt-video-autoresize");
                                    bool bound=false;
                                    if(toggle) {
                                        toggle->click();static_cast<ConeicGlobalSettings*>(settings)->applyOptions();
                                        bound=config_GetInt(intf,"qt-video-autoresize")!=original;
                                        toggle->click();static_cast<ConeicGlobalSettings*>(settings)->applyOptions();
                                        bound=bound && config_GetInt(intf,"qt-video-autoresize")==original;
                                    }
                                    state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Redesigned toggle updates and restores native VLC option")},{QStringLiteral("passed"),bound}});
                                    state->allPassed &= bound;
                                    search->clear();
                                }
                            }
                            bool advanced=QMetaObject::invokeMethod(widget,"setAdvanced",Qt::DirectConnection);
                            app.processEvents();widget->grab().save(directory+QStringLiteral("/settings-full-preferences.png"));
                            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Original full preferences route")},{QStringLiteral("passed"),advanced}});
                            state->allPassed &= advanced;
                            timer->start();
                        }
                        if(current.first=="extendedDialog") {
                            timer->stop();
                            int number=0;
                            std::function<void(QWidget*)> visit;
                            visit=[&](QWidget *root){
                                if(QTabWidget *tabs=qobject_cast<QTabWidget*>(root)) {
                                    const int previous=tabs->currentIndex();
                                    for(int page=0;page<tabs->count();page++) {
                                        tabs->setCurrentIndex(page);app.processEvents();
                                        widget->grab().save(directory+QStringLiteral("/effects-%1.png").arg(number++,2,10,QChar('0')));
                                        visit(tabs->widget(page));
                                    }
                                    tabs->setCurrentIndex(previous);
                                } else {
                                    for(QWidget *child:root->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly))visit(child);
                                }
                            };
                            visit(widget);
                            const bool effects=number>=12;
                            state->results.append(QJsonObject{{QStringLiteral("name"),QStringLiteral("Nested effects tabs visited")},{QStringLiteral("pages"),number},{QStringLiteral("passed"),effects}});
                            state->allPassed &= effects;
                            timer->start();
                        }
                        widget->grab().save(directory + QStringLiteral("/dialog-%1.png").arg(state->index+1,2,10,QChar('0')));
                        if (QDialog *dialog = qobject_cast<QDialog*>(widget)) dialog->reject();
                        else widget->close();
                    }
                }
                state->results.append(QJsonObject{{QStringLiteral("name"),current.second},{QStringLiteral("passed"),visible}});
                state->allPassed &= visible;
                state->awaiting = false;
                state->index++;
            }
        });
        timer->start();
    });
}
}
#endif
