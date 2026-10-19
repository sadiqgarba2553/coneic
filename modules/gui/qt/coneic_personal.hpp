/* Coneic Moments, Study and Smart Collections. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_PERSONAL_HPP
#define CONEIC_PERSONAL_HPP
#include "coneic_personal_store.hpp"
#include "coneic_thumbnails.hpp"
#include "coneic_library_tools.hpp"
#include "recents.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QScrollArea>
#include <QScrollBar>

class ConeicPersonalDialog : public QDialog
{
    intf_thread_t *intf;ConeicPersonalStore *store;
    QTabWidget *tabs;QListWidget *momentList,*noteList,*mediaList;
    QLineEdit *momentSearch,*noteTitle;QPlainTextEdit *note;
    QComboBox *collection;QLabel *status,*context,*range;
    QPushButton *capture,*saveNote,*markStart,*markEnd,*editMoment,*removeMoment,*playMoment,*tagsButton;
    QCheckBox *repeat;
    QString draftUri; qint64 draftStart=0,draftEnd=0;
    QString loopUri; qint64 loopStart=0,loopEnd=0;
    QString pendingUri;qint64 pendingTime=0;int pendingTries=0;
    bool rendering=false;QString viewedUri;
    QByteArray momentSignature,collectionSignature;QString previousMomentQuery;
    QJsonObject selected(QListWidget *list)const{return list->currentItem()?list->currentItem()->data(Qt::UserRole).toJsonObject():QJsonObject();}
    void message(const QString &text,bool failure=false){status->setText(text);status->setProperty("coneicError",failure);status->setAccessibleName(text);}
    void result(bool ok,const QString &success){message(ok?success:store->error,!ok);}
    static QHBoxLayout *row(QVBoxLayout *layout){auto r=new QHBoxLayout;r->setSpacing(12);layout->addLayout(r);return r;}
    static QPushButton *button(const QString &label,QHBoxLayout *layout){auto b=new QPushButton(label);b->setMinimumHeight(28);layout->addWidget(b);return b;}
    void addPage(QWidget *body,const QIcon &icon,const QString &name){auto scroll=new QScrollArea(tabs);scroll->setFrameShape(QFrame::NoFrame);scroll->setWidgetResizable(true);scroll->setMinimumSize(0,0);scroll->setWidget(body);tabs->addTab(scroll,icon,name);}
    static void writeFile(const QString &path,const QByteArray &bytes,bool &ok){QSaveFile file(path);ok=file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();}
    void updateRange(){range->setText(ConeicPersonalStore::stamp(draftStart)+(draftEnd>draftStart?"  -  "+ConeicPersonalStore::stamp(draftEnd):QString()) );}
    void newDraft(){draftUri=ConeicPersonalStore::currentUri(intf);draftStart=ConeicPersonalStore::currentTime(intf);draftEnd=0;noteTitle->clear();note->clear();updateRange();}
    void activate(const QJsonObject &m) {
        const QString uri=store->resolve(m["uri"].toString());if(uri.isEmpty())return;
        const QUrl url=QUrl::fromEncoded(uri.toUtf8());if(url.isLocalFile()&&!QFileInfo::exists(url.toLocalFile())){message(qtr("This file has moved or is unavailable. Your saved note is still kept."),true);return;}
        pendingUri=uri;pendingTime=qint64(m["positionMs"].toDouble());pendingTries=0;
        if(ConeicPersonalStore::currentUri(intf)!=uri)Open::openMRL(intf,uri);
        message(qtr("Opening saved moment…"));
    }
    void tick() {
        const QString uri=ConeicPersonalStore::currentUri(intf);
        if(!pendingUri.isEmpty()) {
            if(uri==pendingUri && ConeicPersonalStore::seekable(intf)) {
                auto input=MainInputManager::getInstance(intf)->getInput();const qint64 length=var_GetInteger(input,"length")/1000;
                const qint64 target=length>0?qMin(pendingTime,qMax<qint64>(0,length-100)):pendingTime;
                var_SetInteger(input,"time",target*1000);pendingUri.clear();if(MainInputManager::getInstance(intf)->getIM()->playingStatus()==PAUSE_S)MainInputManager::getInstance(intf)->togglePlayPause();message(qtr("Jumped to saved moment."));
            }else if(++pendingTries>80){pendingUri.clear();message(qtr("The media could not be opened or does not support seeking."),true);}
        }
        if(!loopUri.isEmpty() && uri==loopUri && repeat->isChecked() && ConeicPersonalStore::seekable(intf) && pendingUri.isEmpty()) {
            if(ConeicPersonalStore::currentTime(intf)>=loopEnd)var_SetInteger(MainInputManager::getInstance(intf)->getInput(),"time",loopStart*1000);
        }
        if(!loopUri.isEmpty() && uri!=loopUri && pendingUri!=loopUri){QSignalBlocker b(repeat);repeat->setChecked(false);loopUri.clear();}
        const bool available=!uri.isEmpty() && ConeicPersonalStore::seekable(intf);
        capture->setEnabled(available);markStart->setEnabled(available);markEnd->setEnabled(available);
        saveNote->setEnabled(!note->toPlainText().trimmed().isEmpty() && (!draftUri.isEmpty() || available));
        context->setText(draftUri.isEmpty()?(uri.isEmpty()?qtr("Open a video or audio file to begin."):ConeicPersonalStore::title(uri)):qtr("Notes for ")+ConeicPersonalStore::title(draftUri));
        if(viewedUri!=uri){viewedUri=uri;renderNotes();}
    }
    void thumbnail(QListWidget *list,QListWidgetItem *item,const QString &uri,qint64 ms) {
        const QUrl url=QUrl::fromEncoded(uri.toUtf8());const QString suffix=QFileInfo(url.path()).suffix().toLower();
        if(!url.isLocalFile() || !QStringLiteral(EXTENSIONS_VIDEO).toLower().split(';').contains("*."+suffix))return;
        const QString identity=item->data(Qt::UserRole+1).toString();QPointer<QListWidget> guard(list);
        ConeicThumbnails::instance(QFileInfo(intf->p_sys->mainSettings->fileName()).absolutePath()+"/thumbnails")->request(url.toLocalFile(),int(ms/1000),[guard,identity](const QString &path){
            if(!guard || path.isEmpty())return;for(int i=0;i<guard->count();i++)if(guard->item(i)->data(Qt::UserRole+1).toString()==identity)guard->item(i)->setIcon(QIcon(path));
        });
    }
    void renderMoments() {
        const QByteArray signature=QJsonDocument(store->moments()).toJson(QJsonDocument::Compact);
        if(signature==momentSignature && previousMomentQuery==momentSearch->text())return;
        momentSignature=signature;previousMomentQuery=momentSearch->text();const int scroll=momentList->verticalScrollBar()->value();
        const QString selectedId=selected(momentList)["id"].toString();momentList->clear();const auto moments=store->moments();
        for(int i=moments.size()-1;i>=0;i--){auto m=moments[i].toObject();
            const QString hay=m["title"].toString()+" "+m["note"].toString()+" "+ConeicPersonalStore::title(m["uri"].toString());
            if(!hay.contains(momentSearch->text(),Qt::CaseInsensitive))continue;
            QString preview=m["note"].toString().simplified();if(preview.size()>110)preview=preview.left(107)+"…";
            auto item=new QListWidgetItem(QIcon(":/coneic/bookmarks.svg"),m["title"].toString()+"\n"+ConeicPersonalStore::title(m["uri"].toString())+"  ·  "+ConeicPersonalStore::stamp(qint64(m["positionMs"].toDouble()))+(preview.isEmpty()?QString():"\n"+preview),momentList);
            item->setData(Qt::UserRole,m);item->setData(Qt::UserRole+1,m["id"].toString());item->setSizeHint(QSize(240,116));
            if(m["id"].toString()==selectedId)momentList->setCurrentItem(item);thumbnail(momentList,item,m["uri"].toString(),qint64(m["positionMs"].toDouble()));
        }
        if(momentList->count()==0)momentList->setToolTip(qtr("Save a moment while media is playing. Your moments stay here after restarting Coneic."));
        updateSelection();
        momentList->verticalScrollBar()->setValue(scroll);
    }
    void updateSelection(){const bool has=!selected(momentList).isEmpty();editMoment->setEnabled(has);removeMoment->setEnabled(has);playMoment->setEnabled(has);tagsButton->setEnabled(mediaList->currentItem()!=nullptr);}
    void renderNotes(){
        QSignalBlocker blocker(noteList);
        const QString id=selected(noteList)["id"].toString();noteList->clear();const QString uri=ConeicPersonalStore::currentUri(intf);
        for(auto v:store->moments()){auto m=v.toObject();if(m["uri"].toString()!=uri || m["note"].toString().isEmpty())continue;
            auto item=new QListWidgetItem(ConeicPersonalStore::stamp(qint64(m["positionMs"].toDouble()))+"  "+m["title"].toString()+"\n"+m["note"].toString().simplified().left(110),noteList);item->setData(Qt::UserRole,m);item->setSizeHint(QSize(240,64));if(m["id"].toString()==id)noteList->setCurrentItem(item);
        }
        const auto m=selected(noteList);repeat->setEnabled(m["endMs"].toDouble()>m["positionMs"].toDouble());
    }
    void renderCollections(){
        const QByteArray signature=QJsonDocument(store->collections()).toJson(QJsonDocument::Compact);
        if(signature==collectionSignature && collection->count()){renderMedia();return;}collectionSignature=signature;
        const QString id=collection->currentData().toJsonObject()["id"].toString();
        {QSignalBlocker blocker(collection);collection->clear();
        const QList<QJsonObject> builtins={{{"id","all"},{"name",qtr("All media")},{"rule","all"}},{{"id","unfinished"},{"name",qtr("Unfinished")},{"rule","unfinished"}},{{"id","week"},{"name",qtr("Added this week")},{"rule","recent"},{"value","7"}},{{"id","short"},{"name",qtr("Under 20 minutes")},{"rule","duration"},{"value","20"}}};
        for(const auto &m:builtins)collection->addItem(m["name"].toString(),m);for(auto m:store->collections())collection->addItem(m.toObject()["name"].toString(),m.toObject());
        for(int i=0;i<collection->count();i++)if(collection->itemData(i).toJsonObject()["id"].toString()==id)collection->setCurrentIndex(i);}
        renderMedia();
    }
    void renderMedia(){
        const QString selectedUri=selected(mediaList)["uri"].toString();const int scroll=mediaList->verticalScrollBar()->value();
        mediaList->clear();const auto c=collection->currentData().toJsonObject();const auto all=store->media();
        for(auto it=all.begin();it!=all.end();++it){auto m=it.value().toObject();if(!ConeicPersonalStore::matches(m,c["rule"].toString(),c["value"].toString()))continue;
            QStringList tags;for(auto t:m["tags"].toArray())tags.append(t.toString());const qint64 duration=qint64(m["durationMs"].toDouble());
            auto item=new QListWidgetItem(QIcon(":/coneic/videos.svg"),m["title"].toString()+"\n"+(duration>0?ConeicPersonalStore::stamp(duration):qtr("Duration not available"))+(tags.isEmpty()?QString():"  ·  "+tags.join(", ")),mediaList);
            item->setData(Qt::UserRole,m);item->setData(Qt::UserRole+1,m["uri"].toString());item->setSizeHint(QSize(240,88));thumbnail(mediaList,item,m["uri"].toString(),1000);
            if(m["uri"].toString()==selectedUri)mediaList->setCurrentItem(item);
        }updateSelection();
        mediaList->verticalScrollBar()->setValue(scroll);
    }
    void render(){if(rendering)return;rendering=true;renderMoments();renderNotes();renderCollections();rendering=false;}
    void editSelected(){
        auto m=selected(momentList);if(m.isEmpty())return;QDialog dialog(this);dialog.setWindowTitle(qtr("Edit moment"));dialog.resize(520,360);
        auto layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,24,24,24);auto title=new QLineEdit(m["title"].toString(),&dialog);title->setAccessibleName(qtr("Moment title"));layout->addWidget(title);
        auto text=new QPlainTextEdit(m["note"].toString(),&dialog);text->setAccessibleName(qtr("Moment note"));layout->addWidget(text);
        auto buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()==QDialog::Accepted){m.insert("title",title->text().trimmed().isEmpty()?qtr("Saved moment"):title->text().trimmed());m.insert("note",text->toPlainText());result(store->saveMoment(m),qtr("Moment updated."));}
    }
    void createCollection(){
        QDialog dialog(this);dialog.setObjectName("ConeicCollectionEditor");dialog.setWindowTitle(qtr("New smart collection"));auto layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,24,24,24);
        auto form=new QFormLayout;auto name=new QLineEdit(&dialog);auto rule=new QComboBox(&dialog);rule->addItem(qtr("Has tag"),"tag");rule->addItem(qtr("Added in the last days"),"recent");rule->addItem(qtr("Shorter than minutes"),"duration");rule->addItem(qtr("Unfinished"),"unfinished");auto value=new QLineEdit(&dialog);value->setPlaceholderText(qtr("Tag name"));form->addRow(qtr("Name"),name);form->addRow(qtr("Rule"),rule);form->addRow(qtr("Value"),value);layout->addLayout(form);
        auto feedback=new QLabel(&dialog);feedback->setWordWrap(true);layout->addWidget(feedback);connect(rule,QOverload<int>::of(&QComboBox::currentIndexChanged),&dialog,[=](){value->setEnabled(rule->currentData().toString()!="unfinished");value->setPlaceholderText(rule->currentData().toString()=="tag"?qtr("Tag name"):qtr("Positive whole number"));});
        name->setObjectName("ConeicCollectionName");rule->setObjectName("ConeicCollectionRule");value->setObjectName("ConeicCollectionValue");
        auto buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        connect(buttons,&QDialogButtonBox::accepted,&dialog,[=,&dialog](){const QString r=rule->currentData().toString();bool valid=true;int n=value->text().toInt(&valid);
            if(name->text().trimmed().isEmpty() || (r=="tag"&&value->text().trimmed().isEmpty()) || ((r=="recent"||r=="duration")&&(!valid||n<=0))){feedback->setText(qtr("Enter a name and a valid rule value."));return;}
            if(store->saveCollection({{"name",name->text().trimmed()},{"rule",r},{"value",value->text().trimmed()}}))dialog.accept();else feedback->setText(store->error);
        });dialog.exec();
    }
public:
    explicit ConeicPersonalDialog(intf_thread_t *p,QWidget *parent=nullptr):QDialog(parent),intf(p),store(ConeicPersonalStore::instance(p)) {
        setObjectName("ConeicPersonalDialog");setWindowTitle(qtr("Your space · Coneic 0.5.0"));setAttribute(Qt::WA_DeleteOnClose);setMinimumSize(560,480);
        const QRect screen=QGuiApplication::primaryScreen()->availableGeometry();resize(qMin(940,screen.width()-48),qMin(720,screen.height()-64));
        auto layout=new QVBoxLayout(this);layout->setContentsMargins(24,20,24,20);layout->setSpacing(16);
        auto headingRow=new QHBoxLayout;auto heading=new QLabel(qtr("Your space"),this);heading->setObjectName("ConeicHomeTitle");headingRow->addWidget(heading,1);auto tools=new QPushButton(qtr("Library tools"),this);headingRow->addWidget(tools);layout->addLayout(headingRow);connect(tools,&QPushButton::clicked,this,[this](){ConeicLibraryTools::open(intf,this);});
        tabs=new QTabWidget(this);tabs->setObjectName("ConeicPersonalTabs");layout->addWidget(tabs,1);
        auto moments=new QWidget(tabs);auto ml=new QVBoxLayout(moments);ml->setContentsMargins(0,16,0,0);ml->setSpacing(16);auto mr=row(ml);
        capture=button(qtr("Save current moment"),mr);capture->setObjectName("ConeicCaptureMoment");momentSearch=new QLineEdit(moments);momentSearch->setPlaceholderText(qtr("Search moments"));momentSearch->setAccessibleName(qtr("Search moments"));mr->addWidget(momentSearch,1);
        momentList=new QListWidget(moments);momentList->setObjectName("ConeicMomentCards");momentList->setIconSize(QSize(160,90));momentList->setSpacing(8);momentList->setMinimumSize(0,80);ml->addWidget(momentList,1);
        auto ma=row(ml);playMoment=button(qtr("Play moment"),ma);editMoment=button(qtr("Edit"),ma);removeMoment=button(qtr("Delete"),ma);ma->addStretch();addPage(moments,QIcon(":/coneic/bookmarks.svg"),qtr("Moments"));
        auto study=new QWidget(tabs);auto sl=new QVBoxLayout(study);sl->setContentsMargins(0,16,0,0);sl->setSpacing(12);context=new QLabel(study);context->setWordWrap(true);sl->addWidget(context);
        noteTitle=new QLineEdit(study);noteTitle->setObjectName("ConeicNoteTitle");noteTitle->setPlaceholderText(qtr("Note title (optional)"));noteTitle->setAccessibleName(qtr("Note title"));sl->addWidget(noteTitle);
        note=new QPlainTextEdit(study);note->setObjectName("ConeicNoteText");note->setPlaceholderText(qtr("Write a note about this moment…"));note->setAccessibleName(qtr("Study note"));note->setMinimumHeight(96);sl->addWidget(note,1);
        auto sr=row(sl);markStart=button(qtr("Mark start"),sr);markStart->setObjectName("ConeicMarkStart");markEnd=button(qtr("Mark end"),sr);markEnd->setObjectName("ConeicMarkEnd");range=new QLabel("00:00:00",study);sr->addWidget(range,1);
        auto sa=row(sl);saveNote=button(qtr("Save note"),sa);saveNote->setObjectName("ConeicSaveNote");auto clear=button(qtr("New note"),sa);sa->addStretch();
        noteList=new QListWidget(study);noteList->setObjectName("ConeicStudyNotes");noteList->setMinimumSize(0,64);sl->addWidget(noteList,1);
        auto sb=row(sl);repeat=new QCheckBox(qtr("Loop selected section"),study);repeat->setObjectName("ConeicStudyLoop");sb->addWidget(repeat,1);auto exportNotes=button(qtr("Export notes"),sb);addPage(study,QIcon(":/coneic/menu-file.svg"),qtr("Study"));
        auto collections=new QWidget(tabs);auto cl=new QVBoxLayout(collections);cl->setContentsMargins(0,16,0,0);cl->setSpacing(16);auto cr=row(cl);collection=new QComboBox(collections);collection->setObjectName("ConeicSmartCollection");collection->setAccessibleName(qtr("Smart collection"));cr->addWidget(collection,1);auto newCollection=button(qtr("New"),cr);newCollection->setObjectName("ConeicNewCollection");auto deleteCollection=button(qtr("Delete"),cr);
        auto info=new QLabel(qtr("Collections update automatically. Local files are scanned for duration in the background; unsupported files keep their notes and tags."),collections);info->setWordWrap(true);cl->addWidget(info);
        mediaList=new QListWidget(collections);mediaList->setObjectName("ConeicCollectionMedia");mediaList->setIconSize(QSize(112,64));mediaList->setSpacing(8);mediaList->setMinimumSize(0,80);cl->addWidget(mediaList,1);auto ca=row(cl);tagsButton=button(qtr("Edit tags"),ca);ca->addStretch();addPage(collections,QIcon(":/coneic/playlists.svg"),qtr("Collections"));
        status=new QLabel(this);status->setObjectName("ConeicPersonalStatus");status->setWordWrap(true);status->setMinimumHeight(24);layout->addWidget(status);
        connect(capture,&QPushButton::clicked,this,[this](){saveCurrentMoment();});
        connect(momentSearch,&QLineEdit::textChanged,this,[this](){renderMoments();});connect(momentList,&QListWidget::itemSelectionChanged,this,[this](){updateSelection();});
        connect(momentList,&QListWidget::itemActivated,this,[this](QListWidgetItem*){activate(selected(momentList));});connect(playMoment,&QPushButton::clicked,this,[this](){activate(selected(momentList));});connect(editMoment,&QPushButton::clicked,this,[this](){editSelected();});
        connect(removeMoment,&QPushButton::clicked,this,[this](){auto m=selected(momentList);if(!m.isEmpty()&&QMessageBox::question(this,qtr("Delete moment"),qtr("Delete this saved moment and its note? The media file will be kept."))==QMessageBox::Yes)result(store->deleteMoment(m["id"].toString()),qtr("Moment deleted."));});
        connect(note,&QPlainTextEdit::textChanged,this,[this](){if(draftUri.isEmpty()&&!note->toPlainText().isEmpty()){draftUri=ConeicPersonalStore::currentUri(intf);draftStart=ConeicPersonalStore::currentTime(intf);updateRange();}tick();});
        connect(markStart,&QPushButton::clicked,this,[this](){draftUri=ConeicPersonalStore::currentUri(intf);draftStart=ConeicPersonalStore::currentTime(intf);draftEnd=0;updateRange();});
        connect(markEnd,&QPushButton::clicked,this,[this](){const qint64 end=ConeicPersonalStore::currentTime(intf);if(draftUri.isEmpty()||draftUri!=ConeicPersonalStore::currentUri(intf)||end<=draftStart){message(qtr("Mark a start, then an end later in the same media file."),true);return;}draftEnd=end;updateRange();});
        connect(clear,&QPushButton::clicked,this,[this](){if(!note->toPlainText().isEmpty()&&QMessageBox::question(this,qtr("New note"),qtr("Discard this unsaved draft?"))!=QMessageBox::Yes)return;newDraft();});
        connect(saveNote,&QPushButton::clicked,this,[this](){if(draftUri.isEmpty()||note->toPlainText().trimmed().isEmpty())return;const bool ok=store->saveMoment({{"uri",draftUri},{"title",noteTitle->text().trimmed().isEmpty()?qtr("Study note"):noteTitle->text().trimmed()},{"note",note->toPlainText()},{"positionMs",double(draftStart)},{"endMs",double(draftEnd)}});if(ok)newDraft();result(ok,qtr("Note saved."));});
        connect(noteList,&QListWidget::itemActivated,this,[this](QListWidgetItem*){activate(selected(noteList));});
        connect(noteList,&QListWidget::itemSelectionChanged,this,[this](){const auto m=selected(noteList);repeat->setEnabled(m["endMs"].toDouble()>m["positionMs"].toDouble());if(repeat->isChecked()){QSignalBlocker b(repeat);repeat->setChecked(false);loopUri.clear();}});
        connect(repeat,&QCheckBox::toggled,this,[this](bool on){loopUri.clear();if(!on)return;auto m=selected(noteList);loopStart=qint64(m["positionMs"].toDouble());loopEnd=qint64(m["endMs"].toDouble());if(loopEnd<=loopStart)return;loopUri=m["uri"].toString();activate(m);});
        connect(exportNotes,&QPushButton::clicked,this,[this](){const QString uri=ConeicPersonalStore::currentUri(intf);if(uri.isEmpty()){message(qtr("Open the media whose notes you want to export."),true);return;}const QString target=QFileDialog::getSaveFileName(this,qtr("Export current media notes"),"Coneic notes.md",qtr("Markdown (*.md)"));if(!target.isEmpty())exportNotesTo(target,uri);});
        connect(collection,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){renderMedia();});connect(newCollection,&QPushButton::clicked,this,[this](){createCollection();});
        connect(deleteCollection,&QPushButton::clicked,this,[this](){auto m=collection->currentData().toJsonObject();const QString id=m["id"].toString();if(id=="all"||id=="unfinished"||id=="week"||id=="short"){message(qtr("Built-in collections are kept. You can delete collections you create."));return;}if(QMessageBox::question(this,qtr("Delete collection"),qtr("Delete this collection? Media, tags and notes will be kept."))==QMessageBox::Yes)result(store->deleteCollection(id),qtr("Collection deleted."));});
        connect(tagsButton,&QPushButton::clicked,this,[this](){const auto m=selected(mediaList);if(m.isEmpty())return;QStringList tags;for(auto t:m["tags"].toArray())tags.append(t.toString());bool ok;QString value=QInputDialog::getText(this,qtr("Media tags"),qtr("Separate tags with commas"),QLineEdit::Normal,tags.join(", "),&ok);if(ok)result(store->setTags(m["uri"].toString(),value.split(',')),qtr("Tags saved."));});
        connect(mediaList,&QListWidget::itemSelectionChanged,this,[this](){updateSelection();});connect(mediaList,&QListWidget::itemActivated,this,[this](QListWidgetItem*){auto m=selected(mediaList);m.insert("positionMs",0);activate(m);});
        store->watch(this,[this](){render();});store->reload();store->flush();render();message(store->error.isEmpty()?qtr("Saved on this PC. Nothing is uploaded."):store->error,!store->error.isEmpty());
        auto timer=new QTimer(this);timer->setInterval(100);connect(timer,&QTimer::timeout,this,[this](){tick();});timer->start();tick();
    }
    bool saveCurrentMoment(){if(!ConeicPersonalStore::seekable(intf)){message(qtr("Open a seekable video or audio file first."),true);return false;}const QString uri=ConeicPersonalStore::currentUri(intf);const qint64 time=ConeicPersonalStore::currentTime(intf);bool ok=store->saveMoment({{"uri",uri},{"title",QString(qtr("Moment at ")+ConeicPersonalStore::stamp(time))},{"note",QString()},{"positionMs",double(time)}});result(ok,qtr("Moment saved. Select Edit to add a title or note."));return ok;}
    bool exportNotesTo(const QString &path,const QString &uri){bool ok;writeFile(path,store->markdown(uri).toUtf8(),ok);message(ok?qtr("Notes exported."):qtr("Could not write the export file."),!ok);return ok;}
    void setPage(int index){tabs->setCurrentIndex(index);}
    void playSaved(const QJsonObject &moment){activate(moment);}
    void selectCollection(const QString &id){setPage(2);for(int i=0;i<collection->count();i++)if(collection->itemData(i).toJsonObject().value("id").toString()==id){collection->setCurrentIndex(i);break;}}
    static ConeicPersonalDialog *open(intf_thread_t *p,QWidget *parent,int page=0){
        ConeicPersonalDialog *dialog=nullptr;for(QWidget *w:qApp->topLevelWidgets())if(w->objectName()=="ConeicPersonalDialog"){dialog=static_cast<ConeicPersonalDialog*>(w);break;}
        if(!dialog)dialog=new ConeicPersonalDialog(p,parent);dialog->setPage(page);dialog->show();dialog->raise();dialog->activateWindow();return dialog;
    }
protected:
    void reject() override {
        if(!note->toPlainText().trimmed().isEmpty() && QMessageBox::question(this,qtr("Unsaved note"),qtr("Close without saving this draft?"))!=QMessageBox::Yes)return;
        loopUri.clear();QDialog::reject();
    }
};
#endif
