/* Local library tools. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_LIBRARY_TOOLS_HPP
#define CONEIC_LIBRARY_TOOLS_HPP
#include "coneic_personal_store.hpp"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
namespace ConeicLibraryTools {
inline bool locate(intf_thread_t *intf,QWidget *parent,const QString &uri) {
    auto store=ConeicPersonalStore::instance(intf);const QString file=QFileDialog::getOpenFileName(parent,qtr("Locate replacement for ")+ConeicPersonalStore::title(uri));if(file.isEmpty())return false;
    const QString replacement=QString::fromUtf8(QUrl::fromLocalFile(file).toEncoded());
    if(QMessageBox::question(parent,qtr("Relink saved references"),qtr("Use this file for the saved moments, notes and library entry? Neither media file will be changed.")+"\n\n"+file)!=QMessageBox::Yes)return false;
    if(!store->relink(uri,replacement)){QMessageBox::warning(parent,qtr("Could not relink"),store->error);return false;}return true;
}
inline void open(intf_thread_t *intf,QWidget *parent) {
    auto store=ConeicPersonalStore::instance(intf);auto dialog=new QDialog(parent);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setObjectName("ConeicLibraryTools");dialog->setWindowTitle(qtr("Library tools"));dialog->resize(640,480);
    auto layout=new QVBoxLayout(dialog);layout->setContentsMargins(24,24,24,24);layout->setSpacing(16);
    auto text=new QLabel(qtr("Back up your notes, moments, tags, collections and Home layout. Media files are not included."),dialog);text->setWordWrap(true);layout->addWidget(text);
    auto row=new QHBoxLayout;auto backup=new QPushButton(qtr("Create backup"),dialog);auto restore=new QPushButton(qtr("Restore backup"),dialog);row->addWidget(backup);row->addWidget(restore);row->addStretch();layout->addLayout(row);
    layout->addWidget(new QLabel(qtr("Missing local media"),dialog));auto list=new QListWidget(dialog);list->setObjectName("ConeicMissingMedia");layout->addWidget(list,1);
    auto locateButton=new QPushButton(qtr("Locate selected file"),dialog);layout->addWidget(locateButton);auto status=new QLabel(dialog);status->setWordWrap(true);layout->addWidget(status);
    auto refresh=[=](){list->clear();const auto media=store->media();for(auto it=media.begin();it!=media.end();++it){const QUrl url=QUrl::fromEncoded(it.key().toUtf8());if(url.isLocalFile()&&!QFileInfo::exists(url.toLocalFile())){auto item=new QListWidgetItem(ConeicPersonalStore::title(it.key()),list);item->setData(Qt::UserRole,it.key());item->setToolTip(url.toLocalFile());}}locateButton->setEnabled(list->currentItem());if(!list->count())status->setText(qtr("No missing local files were found."));};
    QObject::connect(list,&QListWidget::itemSelectionChanged,dialog,[=](){locateButton->setEnabled(list->currentItem());});QObject::connect(locateButton,&QPushButton::clicked,dialog,[=](){if(list->currentItem()&&locate(intf,dialog,list->currentItem()->data(Qt::UserRole).toString()))refresh();});
    QObject::connect(backup,&QPushButton::clicked,dialog,[=](){const QString file=QFileDialog::getSaveFileName(dialog,qtr("Save library backup"),"Coneic library backup.json",qtr("Coneic backup (*.json)"));if(!file.isEmpty())status->setText(store->exportBackup(file)?qtr("Backup saved. Your media files stay in their original locations."):store->error);});
    QObject::connect(restore,&QPushButton::clicked,dialog,[=](){const QString file=QFileDialog::getOpenFileName(dialog,qtr("Restore library backup"),QString(),qtr("Coneic backup (*.json)"));if(file.isEmpty())return;if(QMessageBox::question(dialog,qtr("Merge backup into this library"),qtr("Existing notes and tags will be kept. Different notes sharing an identifier are retained separately. A recovery copy is saved before restoring. Continue?"))!=QMessageBox::Yes)return;status->setText(store->importBackup(file)?qtr("Backup merged. Use Locate selected file if media has moved."):store->error);refresh();});
    refresh();dialog->show();
}
}
#endif
