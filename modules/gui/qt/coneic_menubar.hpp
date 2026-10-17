/* Compact Coneic chrome; original VLC QAction objects and shortcuts retained.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONEIC_MENUBAR_HPP
#define CONEIC_MENUBAR_HPP
#include "input_manager.hpp"
#include "dialogs_provider.hpp"
#include "coneic_personal.hpp"
#include <QMenuBar>
#include <QMenu>
#include <QPainter>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QResizeEvent>
#include <functional>

class ConeicMenuBar : public QMenuBar
{
    QWidget *header=nullptr;
    QMenu *popup;
    intf_thread_t *intf;
    void rebuildMenu()
    {
        popup->clear();
        // Share the real actions. No alternate shortcuts or duplicate behavior.
        const QList<QAction*> original=actions();
        if(!original.isEmpty() && original.first()->menu())popup->addActions(original.first()->menu()->actions());
        popup->addSeparator();
        auto personal=popup->addAction(QIcon(":/coneic/bookmarks.svg"),qtr("Your space · Moments, Study & Collections"));
        connect(personal,&QAction::triggered,this,[this](){ConeicPersonalDialog::open(intf,parentWidget());});
        QAction *settings=popup->addAction(QIcon(":/coneic/menu-settings.svg"),qtr("Settings"));
        connect(settings,&QAction::triggered,DialogsProvider::getInstance(intf),&DialogsProvider::prefsDialog);
        QAction *customize=popup->addAction(QIcon(":/coneic/menu-customize.svg"),qtr("Customize controls"));
        connect(customize,&QAction::triggered,DialogsProvider::getInstance(intf),&DialogsProvider::toolbarDialog);
        QMenu *complete=popup->addMenu(qtr("All controls"));
        for(QAction *action:original)if(action->menu())complete->addMenu(action->menu());
    }
protected:
    void paintEvent(QPaintEvent *) override {QPainter p(this);p.fillRect(rect(),palette().color(QPalette::Window));}
    void resizeEvent(QResizeEvent *event) override
    {
        QMenuBar::resizeEvent(event);if(header){header->setGeometry(rect());header->raise();}
    }
public:
    ConeicMenuBar(intf_thread_t *p,QWidget *parent,std::function<void()> library):QMenuBar(parent),intf(p)
    {
        setObjectName("ConeicMenuBar");setNativeMenuBar(false);setFixedHeight(42);
        header=new QWidget(this);header->setObjectName("ConeicChrome");
        header->setAttribute(Qt::WA_NoMousePropagation,true);
        QHBoxLayout *layout=new QHBoxLayout(header);layout->setContentsMargins(10,3,12,3);layout->setSpacing(10);
        QToolButton *menu=new QToolButton(header);menu->setText(QString(QChar(0x2630)));menu->setToolTip(qtr("Menu"));menu->setAccessibleName(qtr("Menu"));menu->setFixedSize(32,32);
        popup=new QMenu(menu);menu->setMenu(popup);menu->setPopupMode(QToolButton::InstantPopup);
        connect(popup,&QMenu::aboutToShow,this,[this](){rebuildMenu();});layout->addWidget(menu);
        QToolButton *home=new QToolButton(header);home->setIcon(QIcon(":/coneic/home.svg"));home->setToolTip(qtr("Open library"));home->setAccessibleName(qtr("Open library"));home->setFixedSize(32,32);
        connect(home,&QToolButton::clicked,this,[library](){library();});layout->addWidget(home);
        QLabel *title=new QLabel(QStringLiteral("Coneic 0.2.1 Alpha"),header);title->setTextFormat(Qt::PlainText);title->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(title,1);
        connect(MainInputManager::getInstance(intf)->getIM(),&InputManager::nameChanged,title,[title](const QString &name){title->setText(name.isEmpty()?QStringLiteral("Coneic 0.2.1 Alpha"):name);});
        auto space=new QToolButton(header);space->setIcon(QIcon(":/coneic/bookmarks.svg"));space->setAccessibleName(qtr("Your space"));space->setToolTip(qtr("Your space: Moments, Study and Collections"));space->setFixedSize(32,32);layout->addWidget(space);connect(space,&QToolButton::clicked,this,[this](){ConeicPersonalDialog::open(intf,parentWidget());});
        auto add=new QToolButton(header);add->setIcon(QIcon(":/coneic/add.svg"));add->setToolTip(qtr("Open media"));add->setAccessibleName(qtr("Open media"));add->setFixedSize(32,32);
        connect(add,&QToolButton::clicked,DialogsProvider::getInstance(intf),&DialogsProvider::simpleOpenDialog);layout->addWidget(add);
        header->setGeometry(rect());header->show();header->raise();
    }
    QSize sizeHint() const override {return QSize(600,42);}
    QSize minimumSizeHint() const override {return QSize(240,42);}
};
#endif
