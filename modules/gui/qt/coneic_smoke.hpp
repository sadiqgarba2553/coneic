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
#include <memory>
#include <functional>
#include "dialogs_provider.hpp"

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
                QJsonObject result{{QStringLiteral("passed"),state->allPassed},
                    {QStringLiteral("checks"),state->results},
                    {QStringLiteral("scope"),QStringLiteral("Native UI opening, rendering, branding and menu inventory. Devices, network transmission and protected discs are not exercised.")}};
                QFile file(directory + QStringLiteral("/native-ui-results.json"));
                if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(result).toJson()); file.close(); }
                DialogsProvider::getInstance(intf)->quit();
                timer->deleteLater();
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
