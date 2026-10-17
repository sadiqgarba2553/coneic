/* Opt-in synthetic acceptance checks. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_PERSONAL_SMOKE_HPP
#define CONEIC_PERSONAL_SMOKE_HPP
#include "coneic_personal.hpp"
#include <QTemporaryDir>
#include <memory>
namespace ConeicPersonalSmoke {
inline void persistence(const QString &directory) {
    QJsonArray results;auto check=[&](const QString &name,bool pass){results.append(QJsonObject{{"name",name},{"passed",pass}});};
    QTemporaryDir temp;const QString path=temp.path()+"/personal.json";
    ConeicPersonalStore a(nullptr,path),b(nullptr,path);const QString uri="file:///C:/Synthetic%20media/lecture.mp4";
    check("New catalog persists",a.catalog({uri})&&a.media().contains(uri));
    check("Tags trim and deduplicate",a.setTags(uri,{" Learning ","learning","Research",""})&&a.media()[uri].toObject()["tags"].toArray().size()==2);
    QJsonObject note{{"id","fixture-note"},{"uri",uri},{"title",QString::fromUtf8("Research · 中文")},{"note",QString::fromUtf8("Line one\nLine two — café")},{"positionMs",1200.},{"endMs",3400.}};
    check("Unicode timestamped note saves",a.saveMoment(note));
    check("Second writer merges latest data",b.catalog({"file:///C:/Synthetic%20media/second.mp4"})&&b.moments().size()==1&&b.media()[uri].toObject()["tags"].toArray().size()==2);
    ConeicPersonalStore again(nullptr,path);
    check("Restart retains notes and Unicode",again.moments().size()==1&&again.moments()[0].toObject()["note"]==note["note"]&&again.media().size()==2);
    note.insert("title","Edited title");check("Editing replaces one record",again.saveMoment(note)&&again.moments().size()==1&&again.moments()[0].toObject()["title"].toString()=="Edited title");
    check("Invalid section is rejected",!again.saveMoment({{"uri",uri},{"positionMs",4000.},{"endMs",2000.}}));
    QJsonObject shortMedia{{"durationMs",600000.},{"positionMs",10000.},{"addedMs",double(QDateTime::currentMSecsSinceEpoch())},{"tags",QJsonArray({"Learning"})}};
    check("Unfinished excludes completed media",ConeicPersonalStore::matches(shortMedia,"unfinished",{})&&!ConeicPersonalStore::matches(QJsonObject{{"durationMs",600000.},{"positionMs",590000.}},"unfinished",{}));
    check("Duration filter excludes unknown duration",ConeicPersonalStore::matches(shortMedia,"duration","20")&&!ConeicPersonalStore::matches(QJsonObject(),"duration","20"));
    check("Tag and recent rules select matching media",ConeicPersonalStore::matches(shortMedia,"recent","7")&&ConeicPersonalStore::matches(shortMedia,"tag","learning")&&!ConeicPersonalStore::matches(shortMedia,"tag","other"));
    check("Custom collection saves",again.saveCollection({{"id","fixture-collection"},{"name","My lessons"},{"rule","tag"},{"value","Learning"}}));
    ConeicPersonalStore restarted(nullptr,path);check("Custom collection survives restart",restarted.collections().size()==1);
    const bool generated=restarted.saveCollection({{"name","Auto identifier"},{"rule","tag"},{"value","Learning"}});const auto last=restarted.collections().last().toObject();check("New UI collections receive persistent identifiers",generated&&!last["id"].toString().isEmpty());restarted.deleteCollection(last["id"].toString());
    const QString markdown=restarted.markdown(uri);check("Markdown retains full note and section times",markdown.contains(note["note"].toString())&&markdown.contains("00:00:01 - 00:00:03")&&markdown.contains("Edited title"));
    QLockFile held(path+".lock");const bool locked=held.tryLock(0);check("Concurrent writer lock protects data",locked&&!restarted.deleteMoment("fixture-note"));held.unlock();
    check("Deleting a moment keeps media and tags",restarted.deleteMoment("fixture-note")&&restarted.moments().isEmpty()&&restarted.media()[uri].toObject()["tags"].toArray().size()==2);
    check("Deleting a collection keeps media",restarted.deleteCollection("fixture-collection")&&restarted.collections().isEmpty()&&restarted.media().size()==2);
    const QString damaged=temp.path()+"/damaged.json";QFile f(damaged);f.open(QIODevice::WriteOnly);f.write("{incomplete");f.close();
    ConeicPersonalStore corrupt(nullptr,damaged);check("Corrupt data refuses writes",!corrupt.catalog({uri}));f.open(QIODevice::ReadOnly);check("Corrupt bytes remain recoverable",f.readAll()=="{incomplete");f.close();
    bool pass=true;for(auto v:results)pass&=v.toObject()["passed"].toBool();
    QFile report(directory+"/personal-persistence-results.json");if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(QJsonObject{{"passed",pass},{"checks",results}}).toJson());qApp->setProperty("coneicPersonalPersistencePassed",pass);
}
inline void runtime(intf_thread_t *intf,const QString &directory,std::function<void(bool)> complete) {
    auto results=std::make_shared<QJsonArray>();auto check=[results](const QString &name,bool pass){results->append(QJsonObject{{"name",name},{"passed",pass}});};
    QWidget *main=nullptr;for(QWidget *w:qApp->topLevelWidgets())if(w->inherits("QMainWindow")){main=w;break;}
    auto dialog=ConeicPersonalDialog::open(intf,main);auto store=ConeicPersonalStore::instance(intf);
    check("Moment capture binds to native input",dialog->saveCurrentMoment());
    auto input=MainInputManager::getInstance(intf)->getInput();if(input)var_SetInteger(input,"time",2000000);
    QTimer::singleShot(500,dialog,[=](){
        dialog->setPage(1);dialog->findChild<QPushButton*>("ConeicMarkStart")->click();
        dialog->findChild<QLineEdit*>("ConeicNoteTitle")->setText("Motion study");dialog->findChild<QPlainTextEdit*>("ConeicNoteText")->setPlainText("Observe the orange circle and colour gradient.\nA timestamped study note.");
        auto input=MainInputManager::getInstance(intf)->getInput();if(input)var_SetInteger(input,"time",4000000);
        QTimer::singleShot(500,dialog,[=](){
            dialog->findChild<QPushButton*>("ConeicMarkEnd")->click();dialog->findChild<QPushButton*>("ConeicSaveNote")->click();
            auto notes=dialog->findChild<QListWidget*>("ConeicStudyNotes");const bool saved=notes&&notes->count()>0;QJsonObject m;
            if(saved){notes->setCurrentRow(notes->count()-1);m=notes->currentItem()->data(Qt::UserRole).toJsonObject();}
            check("Study UI saves start and end timestamps",saved&&m["endMs"].toDouble()>m["positionMs"].toDouble()&&m["note"].toString().contains("timestamped"));dialog->grab().save(directory+"/alpha-study.png");
            auto loop=dialog->findChild<QCheckBox*>("ConeicStudyLoop");if(loop&&loop->isEnabled())loop->click();
            auto wraps=std::make_shared<int>(0);auto previous=std::make_shared<qint64>(-1);auto ticks=std::make_shared<int>(0);auto poll=new QTimer(dialog);poll->setInterval(100);
            QObject::connect(poll,&QTimer::timeout,dialog,[=](){
                const qint64 now=ConeicPersonalStore::currentTime(intf);if(*previous>0&&now+350<*previous)(*wraps)++;*previous=now;
                if(++*ticks<45)return;poll->stop();poll->deleteLater();
                check("Saved section loops through native playback",loop&&loop->isChecked()&&*wraps>0);if(loop&&loop->isChecked())loop->click();
                check("Markdown contains saved study note",store->markdown(ConeicPersonalStore::currentUri(intf)).contains("Observe the orange circle"));
                const QString uri=ConeicPersonalStore::currentUri(intf);check("Native metadata scan discovers duration",store->media()[uri].toObject()["durationMs"].toDouble()>0);check("Live library tags persist",store->setTags(uri,{"Learning","Demo"}));
                const QString exportPath=directory+"/alpha-study-notes.md";const bool exported=dialog->exportNotesTo(exportPath,uri);QFile exportedFile(exportPath);exportedFile.open(QIODevice::ReadOnly);check("Export writes complete Markdown to disk",exported&&QString::fromUtf8(exportedFile.readAll()).contains("Observe the orange circle"));exportedFile.close();
                bool metadata=true;const auto all=store->media();for(auto v:all)metadata &= v.toObject()["durationMs"].toDouble()>0;check("Background parsing discovers unplayed fixture durations",metadata&&all.size()>=3);
                dialog->setPage(0);qApp->processEvents();dialog->grab().save(directory+"/alpha-moments.png");dialog->setPage(2);qApp->processEvents();dialog->grab().save(directory+"/alpha-collections.png");
                QTimer::singleShot(100,dialog,[=](){for(QWidget *w:qApp->topLevelWidgets())if(w->objectName()=="ConeicCollectionEditor"){w->findChild<QLineEdit*>("ConeicCollectionName")->setText("Learning collection");w->findChild<QLineEdit*>("ConeicCollectionValue")->setText("Learning");w->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();}});
                dialog->findChild<QPushButton*>("ConeicNewCollection")->click();
                auto combo=dialog->findChild<QComboBox*>("ConeicSmartCollection");bool custom=false;for(int i=0;i<combo->count();i++)if(combo->itemText(i)=="Learning collection"){combo->setCurrentIndex(i);custom=true;break;}
                check("Custom rule editor creates a live filtered collection",custom&&dialog->findChild<QListWidget*>("ConeicCollectionMedia")->count()==1);
                bool fits=true;QJsonArray sizes;for(auto size:QList<QSize>({QSize(640,600),QSize(1280,800)})){dialog->resize(size);qApp->processEvents();fits &= dialog->width()<=size.width()&&dialog->height()<=size.height();sizes.append(QJsonObject{{"requestedWidth",size.width()},{"requestedHeight",size.height()},{"actualWidth",dialog->width()},{"actualHeight",dialog->height()},{"minimumHeight",dialog->minimumSizeHint().height()}});dialog->grab().save(directory+QString("/alpha-space-%1.png").arg(size.width()));}check("Workspace fits compact and large windows",fits);
                QFile geometry(directory+"/personal-layout-results.json");if(geometry.open(QIODevice::WriteOnly))geometry.write(QJsonDocument(sizes).toJson());
                bool passed=qApp->property("coneicPersonalPersistencePassed").toBool();for(auto v:*results)passed&=v.toObject()["passed"].toBool();
                QFile f(directory+"/personal-runtime-results.json");if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(QJsonObject{{"passed",passed},{"checks",*results}}).toJson());dialog->close();complete(passed);
            });poll->start();
        });
    });
}
}
#endif
