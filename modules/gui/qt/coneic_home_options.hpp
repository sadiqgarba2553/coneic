/* Home layout editor. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_HOME_OPTIONS_HPP
#define CONEIC_HOME_OPTIONS_HPP
#include "coneic_personal_store.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
namespace ConeicHomeOptions {
inline QStringList defaults(){return {"0","6","7","1","2","3","4","5"};}
inline QStringList names(){return {qtr("Continue Watching"),qtr("Playlists"),qtr("Videos"),qtr("Music"),qtr("Streams"),qtr("Other media"),qtr("Saved moments"),qtr("Pinned collections")};}
inline void open(intf_thread_t *intf,QWidget *parent) {
    auto store=ConeicPersonalStore::instance(intf);auto dialog=new QDialog(parent);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setObjectName("ConeicHomeEditor");dialog->setWindowTitle(qtr("Customise Home"));dialog->resize(600,600);
    auto layout=new QVBoxLayout(dialog);layout->setContentsMargins(24,24,24,24);layout->setSpacing(12);auto hint=new QLabel(qtr("Choose sections, then drag them into order. Hidden sections keep their media and notes."),dialog);hint->setWordWrap(true);layout->addWidget(hint);
    auto sections=new QListWidget(dialog);sections->setObjectName("ConeicHomeSections");sections->setMinimumHeight(80);sections->setDragDropMode(QAbstractItemView::InternalMove);layout->addWidget(sections,1);
    const auto prefs=store->home();QStringList order;for(auto v:prefs.value("sections").toArray())if(defaults().contains(v.toString())&&!order.contains(v.toString()))order.append(v.toString());if(order.isEmpty())order=defaults();for(const auto &id:defaults())if(!order.contains(id))order.append(id);
    QStringList hidden;for(auto v:prefs.value("hidden").toArray())hidden.append(v.toString());
    for(const QString &id:order){auto item=new QListWidgetItem(names().at(id.toInt()),sections);item->setData(Qt::UserRole,id);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(hidden.contains(id)?Qt::Unchecked:Qt::Checked);}
    auto moves=new QHBoxLayout;auto up=new QPushButton(qtr("Move up"),dialog);auto down=new QPushButton(qtr("Move down"),dialog);moves->addWidget(up);moves->addWidget(down);moves->addStretch();layout->addLayout(moves);
    QObject::connect(up,&QPushButton::clicked,dialog,[=](){int i=sections->currentRow();if(i>0){auto item=sections->takeItem(i);sections->insertItem(i-1,item);sections->setCurrentItem(item);}});QObject::connect(down,&QPushButton::clicked,dialog,[=](){int i=sections->currentRow();if(i>=0&&i<sections->count()-1){auto item=sections->takeItem(i);sections->insertItem(i+1,item);sections->setCurrentItem(item);}});
    layout->addWidget(new QLabel(qtr("Pin collections to Home"),dialog));auto pins=new QListWidget(dialog);pins->setObjectName("ConeicHomePins");pins->setMinimumHeight(64);layout->addWidget(pins,1);
    QJsonArray choices{QJsonObject{{"id","all"},{"name",qtr("All media")}},QJsonObject{{"id","unfinished"},{"name",qtr("Unfinished")}},QJsonObject{{"id","week"},{"name",qtr("Added this week")}},QJsonObject{{"id","short"},{"name",qtr("Under 20 minutes")}}};for(auto c:store->collections())choices.append(c);
    QStringList selected;for(auto v:prefs.value("pinnedCollections").toArray())selected.append(v.toString());for(auto c:choices){auto m=c.toObject();auto item=new QListWidgetItem(m.value("name").toString(),pins);item->setData(Qt::UserRole,m.value("id").toString());item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(selected.contains(m.value("id").toString())?Qt::Checked:Qt::Unchecked);}
    auto status=new QLabel(dialog);status->setWordWrap(true);layout->addWidget(status);auto buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel|QDialogButtonBox::RestoreDefaults,dialog);layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);QObject::connect(buttons->button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,dialog,[=](){if(store->saveHome({}))dialog->accept();else status->setText(store->error);});
    QObject::connect(buttons,&QDialogButtonBox::accepted,dialog,[=](){QStringList order,hidden,pinned;for(int i=0;i<sections->count();i++){auto item=sections->item(i);const auto id=item->data(Qt::UserRole).toString();order.append(id);if(item->checkState()!=Qt::Checked)hidden.append(id);}for(int i=0;i<pins->count();i++)if(pins->item(i)->checkState()==Qt::Checked)pinned.append(pins->item(i)->data(Qt::UserRole).toString());if(store->saveHome({{"sections",QJsonArray::fromStringList(order)},{"hidden",QJsonArray::fromStringList(hidden)},{"pinnedCollections",QJsonArray::fromStringList(pinned)}}))dialog->accept();else status->setText(store->error);});dialog->show();
}
}
#endif
