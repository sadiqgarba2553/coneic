/* Coneic personal library data. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_PERSONAL_STORE_HPP
#define CONEIC_PERSONAL_STORE_HPP
#include "qt.hpp"
#include "input_manager.hpp"
#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QLockFile>
#include <QFileInfo>
#include <QDir>
#include <QPointer>
#include <QTimer>
#include <QDateTime>
#include <QUuid>
#include <QQueue>
#include <QSet>
#include <QMap>
#include <QSettings>
#include <QUrl>
#include <functional>

class ConeicPersonalStore : public QObject
{
    QString path; QJsonObject data;
    struct Watch {QPointer<QObject> owner;std::function<void()> call;};
    QList<Watch> watches;
    QMap<QString,QJsonObject> progress;
    intf_thread_t *intf;
    QQueue<QString> parseQueue;QSet<QString> queued;
    input_item_t *parsing=nullptr;QString parsingUri; qint64 parseStarted=0;
    bool shuttingDown=false;
    static QJsonObject blank(){return QJsonObject{{"version",1},{"moments",QJsonArray()},{"media",QJsonObject()},{"collections",QJsonArray()}};}
    bool read(QJsonObject &out) {
        if(!QFileInfo::exists(path)){out=blank();return true;}
        QFile f(path);if(!f.open(QIODevice::ReadOnly)){error=qtr("Cannot read your personal library. Your saved data has not been replaced.");return false;}
        QJsonParseError e;auto doc=QJsonDocument::fromJson(f.readAll(),&e);
        if(e.error!=QJsonParseError::NoError || !doc.isObject() || doc.object().value("version").toInt()!=1 || !doc.object().value("moments").isArray() || !doc.object().value("media").isObject() || !doc.object().value("collections").isArray()) {
            error=qtr("The personal library file is invalid or from a newer version. Your saved data has not been replaced.");return false;
        }
        out=doc.object();return true;
    }
    void notify(){const auto copy=watches;for(const Watch &w:copy)if(w.owner)w.call();}
    bool change(const std::function<void(QJsonObject&)> &edit) {
        QDir().mkpath(QFileInfo(path).absolutePath());QLockFile lock(path+".lock");
        if(!lock.tryLock(250)){error=qtr("Your library is busy in another Coneic window. Please try again.");return false;}
        QJsonObject latest;if(!read(latest))return false;edit(latest);
        QSaveFile f(path);const QByteArray bytes=QJsonDocument(latest).toJson();
        if(!f.open(QIODevice::WriteOnly) || f.write(bytes)!=bytes.size() || !f.commit()){error=qtr("Your changes could not be saved. Check available disk space and folder access.");return false;}
        data=latest;error.clear();notify();return true;
    }
    void parseTick() {
        if(shuttingDown)return;
        if(parsing) {
            const qint64 duration=input_item_GetDuration(parsing)/1000;
            if(duration>0 || QDateTime::currentMSecsSinceEpoch()-parseStarted>4500) {
                const QString uri=parsingUri;
                libvlc_MetadataCancel(intf->obj.libvlc,this);input_item_Release(parsing);parsing=nullptr;
                if(duration>0)change([=](QJsonObject &root){auto media=root["media"].toObject();auto m=media[uri].toObject();m.insert("durationMs",double(duration));media.insert(uri,m);root.insert("media",media);});
            }
        }
        if(!parsing && !parseQueue.isEmpty()) {
            parsingUri=parseQueue.dequeue();const QByteArray uri=parsingUri.toUtf8();parsing=input_item_New(uri.constData(),nullptr);
            if(parsing && libvlc_MetadataRequest(intf->obj.libvlc,parsing,META_REQUEST_OPTION_SCOPE_LOCAL,3000,this)!=VLC_SUCCESS){input_item_Release(parsing);parsing=nullptr;}
            parseStarted=QDateTime::currentMSecsSinceEpoch();
        }
    }
public:
    QString error;
    QJsonObject home()const{return data.value("home").toObject();}
    bool saveHome(const QJsonObject &preferences){return change([=](QJsonObject &root){root.insert("home",preferences);});}
    QString resolve(QString uri)const {
        const auto aliases=data.value("relocations").toObject();QSet<QString> seen;
        while(aliases.contains(uri)&&!seen.contains(uri)){seen.insert(uri);uri=aliases.value(uri).toString();}return uri;
    }
    bool relink(const QString &oldUri,const QString &replacement) {
        const QUrl url=QUrl::fromEncoded(replacement.toUtf8());
        if(oldUri.isEmpty() || oldUri==replacement || !url.isLocalFile() || !QFileInfo(url.toLocalFile()).isFile()){error=qtr("Choose an existing replacement media file.");return false;}
        if(!flush())return false;
        const bool ok=change([=](QJsonObject &root){
            auto media=root.value("media").toObject();auto old=media.value(oldUri).toObject();auto current=media.value(replacement).toObject();
            for(auto it=current.begin();it!=current.end();++it)if(!old.contains(it.key()))old.insert(it.key(),it.value());
            QStringList tags;for(auto value:old.value("tags").toArray())tags.append(value.toString());for(auto value:current.value("tags").toArray())if(!tags.contains(value.toString(),Qt::CaseInsensitive))tags.append(value.toString());
            old.insert("tags",QJsonArray::fromStringList(tags));old.insert("uri",replacement);old.insert("title",title(replacement));media.remove(oldUri);media.insert(replacement,old);root.insert("media",media);
            auto notes=root.value("moments").toArray();for(int i=0;i<notes.size();i++){auto m=notes.at(i).toObject();if(m.value("uri").toString()==oldUri){m.insert("uri",replacement);notes.replace(i,m);}}root.insert("moments",notes);
            auto aliases=root.value("relocations").toObject();for(const QString &key:aliases.keys())if(aliases.value(key).toString()==oldUri)aliases.insert(key,replacement);
            aliases.insert(oldUri,replacement);aliases.remove(replacement);root.insert("relocations",aliases);
        });
        if(ok){queued.remove(replacement);catalog({replacement});}return ok;
    }
    static bool validLibrary(const QJsonObject &root) {
        if(root.value("version").toInt()!=1 || !root.value("media").isObject() || !root.value("moments").isArray() || !root.value("collections").isArray())return false;
        for(auto v:root.value("moments").toArray()){if(!v.isObject())return false;auto m=v.toObject();if(m.value("id").toString().isEmpty()||m.value("uri").toString().isEmpty()||!m.value("positionMs").isDouble()||m.value("positionMs").toDouble()<0||!m.value("title").isString()||!m.value("note").isString())return false;const double end=m.value("endMs").toDouble();if(end>0&&end<=m.value("positionMs").toDouble())return false;}
        const auto media=root.value("media").toObject();for(auto it=media.begin();it!=media.end();++it)if(!it.value().isObject()||it.key().isEmpty()||it.value().toObject().value("uri").toString()!=it.key())return false;
        for(auto v:root.value("collections").toArray()){auto c=v.toObject();if(c.value("id").toString().isEmpty()||c.value("name").toString().isEmpty()||!QStringList({"tag","recent","duration","unfinished"}).contains(c.value("rule").toString()))return false;}
        return true;
    }
    bool exportBackup(const QString &destination) {
        const QFileInfo target(destination),active(path);const QString targetPath=target.exists()?target.canonicalFilePath():target.absoluteFilePath();const QString activePath=active.exists()?active.canonicalFilePath():active.absoluteFilePath();
        if(targetPath.compare(activePath,Qt::CaseInsensitive)==0){error=qtr("Choose a backup file outside the active library file.");return false;}
        if(!flush())return false;QJsonObject latest;if(!read(latest))return false;
        const QJsonObject wrapper{{"format","coneic-library-backup"},{"version",1},{"createdMs",double(QDateTime::currentMSecsSinceEpoch())},{"library",latest}};
        QSaveFile file(destination);const auto bytes=QJsonDocument(wrapper).toJson();if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){error=qtr("Could not write the backup file.");return false;}error.clear();return true;
    }
    bool importBackup(const QString &source) {
        QFile file(source);if(file.size()>32*1024*1024 || !file.open(QIODevice::ReadOnly)){error=qtr("Choose a readable Coneic backup smaller than 32 MB.");return false;}
        QJsonParseError parse;const auto doc=QJsonDocument::fromJson(file.readAll(),&parse);const auto wrapper=doc.object();const auto incoming=wrapper.value("library").toObject();
        if(parse.error!=QJsonParseError::NoError || wrapper.value("format").toString()!="coneic-library-backup" || wrapper.value("version").toInt()!=1 || !validLibrary(incoming)){error=qtr("This backup is invalid or uses an unsupported version. Your library has not been replaced.");return false;}
        if(!exportBackup(path+".before-restore-"+QUuid::createUuid().toString()+".json"))return false;
        return change([=](QJsonObject &root){
            auto media=root.value("media").toObject();const auto other=incoming.value("media").toObject();
            for(auto it=other.begin();it!=other.end();++it){auto current=media.value(it.key()).toObject();auto added=it.value().toObject();if(current.isEmpty()){media.insert(it.key(),added);continue;}for(auto field=added.begin();field!=added.end();++field)if(!current.contains(field.key()))current.insert(field.key(),field.value());QStringList tags;for(auto t:current.value("tags").toArray())tags.append(t.toString());for(auto t:added.value("tags").toArray())if(!tags.contains(t.toString(),Qt::CaseInsensitive))tags.append(t.toString());current.insert("tags",QJsonArray::fromStringList(tags));media.insert(it.key(),current);}root.insert("media",media);
            for(const QString &key:QStringList({"moments","collections"})){auto target=root.value(key).toArray();for(auto v:incoming.value(key).toArray()){auto row=v.toObject();bool duplicate=false,collision=false;auto content=row;content.remove("id");for(auto existing:target){auto old=existing.toObject();collision|=old.value("id")==row.value("id");old.remove("id");if(old==content){duplicate=true;break;}}if(!duplicate){if(collision)row.insert("id",QUuid::createUuid().toString());target.append(row);}}root.insert(key,target);}
            if(!root.contains("home")&&incoming.value("home").isObject())root.insert("home",incoming.value("home"));
            auto aliases=root.value("relocations").toObject();const auto incomingAliases=incoming.value("relocations").toObject();for(auto it=incomingAliases.begin();it!=incomingAliases.end();++it)if(!aliases.contains(it.key())&&it.value().isString())aliases.insert(it.key(),it.value());root.insert("relocations",aliases);
        });
    }
    static QString title(const QString &uri){const QUrl url=QUrl::fromEncoded(uri.toUtf8());QString s=QFileInfo(url.path()).completeBaseName();return s.isEmpty()?url.host():s;}
    static QString stamp(qint64 ms){ms=qMax<qint64>(0,ms);return QString("%1:%2:%3").arg(ms/3600000,2,10,QChar('0')).arg(ms/60000%60,2,10,QChar('0')).arg(ms/1000%60,2,10,QChar('0'));}
    static QString currentUri(intf_thread_t *p){input_item_t *item=MainInputManager::getInstance(p)->currentInputItem();if(!item)return {};char *u=input_item_GetURI(item);QString uri=qfu(u);free(u);return uri;}
    static qint64 currentTime(intf_thread_t *p){auto input=MainInputManager::getInstance(p)->getInput();return input?qMax<qint64>(0,var_GetInteger(input,"time")/1000):0;}
    static bool seekable(intf_thread_t *p){auto input=MainInputManager::getInstance(p)->getInput();return input && var_GetBool(input,"can-seek");}
    explicit ConeicPersonalStore(intf_thread_t *p,const QString &file,QObject *parent=nullptr):QObject(parent),path(file),intf(p) {
        setObjectName("ConeicPersonalStore");data=blank();read(data);
        if(!p)return; // Isolated acceptance fixtures can exercise persistence without a player.
        auto im=MainInputManager::getInstance(p)->getIM();
        connect(im,&InputManager::positionUpdated,this,[this](float pos,int64_t time,int length){
            if(pos<0 || time<0)return;const QString uri=resolve(currentUri(intf));if(uri.isEmpty())return;
            auto m=data["media"].toObject()[uri].toObject();m.insert("uri",uri);m.insert("title",title(uri));
            if(!m.contains("addedMs"))m.insert("addedMs",double(QDateTime::currentMSecsSinceEpoch()));
            m.insert("positionMs",double(time/1000));if(length>0)m.insert("durationMs",double(qint64(length)*1000));
            progress[uri]=m;
        });
        auto flushTimer=new QTimer(this);flushTimer->setInterval(15000);connect(flushTimer,&QTimer::timeout,this,[this](){flush();});flushTimer->start();
        auto parseTimer=new QTimer(this);parseTimer->setInterval(150);connect(parseTimer,&QTimer::timeout,this,[this](){parseTick();});parseTimer->start();
        connect(qApp,&QCoreApplication::aboutToQuit,this,[this](){flush();shuttingDown=true;libvlc_MetadataCancel(intf->obj.libvlc,this);if(parsing){input_item_Release(parsing);parsing=nullptr;}});
    }
    static ConeicPersonalStore *instance(intf_thread_t *p) {
        auto found=qApp->findChild<QObject*>(QStringLiteral("ConeicPersonalStore"));
        return found?static_cast<ConeicPersonalStore*>(found):new ConeicPersonalStore(p,QFileInfo(p->p_sys->mainSettings->fileName()).absolutePath()+"/personal-library.json",qApp);
    }
    void watch(QObject *owner,std::function<void()> callback){for(int i=watches.size()-1;i>=0;i--)if(!watches[i].owner)watches.removeAt(i);watches.append({owner,callback});}
    QJsonArray moments()const{return data["moments"].toArray();}
    QJsonObject media()const{return data["media"].toObject();}
    QJsonArray collections()const{return data["collections"].toArray();}
    bool reload(){QJsonObject latest;if(!read(latest))return false;data=latest;notify();return true;}
    bool flush(){if(progress.isEmpty())return true;const auto pending=progress;
        bool ok=change([=](QJsonObject &root){auto media=root["media"].toObject();for(auto i=pending.begin();i!=pending.end();++i){auto m=media[i.key()].toObject();for(auto j=i.value().begin();j!=i.value().end();++j)if(j.key()!="tags")m.insert(j.key(),j.value());media.insert(i.key(),m);}root.insert("media",media);});
        if(ok)progress.clear();return ok;
    }
    bool catalog(const QStringList &uris) {
        QStringList missing;const auto media=this->media();for(const QString &uri:uris)if(!uri.isEmpty()&&!media.contains(uri))missing.append(uri);
        bool ok=missing.isEmpty() || change([=](QJsonObject &root){auto all=root["media"].toObject();for(const QString &uri:missing)if(!all.contains(uri))all.insert(uri,QJsonObject{{"uri",uri},{"title",title(uri)},{"addedMs",double(QDateTime::currentMSecsSinceEpoch())},{"tags",QJsonArray()}});root.insert("media",all);});
        if(ok && intf)for(const QString &uri:uris)if(QUrl::fromEncoded(uri.toUtf8()).isLocalFile() && this->media()[uri].toObject()["durationMs"].toDouble()<=0 && !queued.contains(uri)){queued.insert(uri);parseQueue.enqueue(uri);}
        return ok;
    }
    bool saveMoment(QJsonObject moment) {
        if(moment["uri"].toString().isEmpty() || moment["positionMs"].toDouble(-1)<0){error=qtr("Choose a media file and a valid timestamp.");return false;}
        if(moment["endMs"].toDouble()>0 && moment["endMs"].toDouble()<=moment["positionMs"].toDouble()){error=qtr("The section end must be after its start.");return false;}
        if(moment["id"].toString().isEmpty())moment.insert("id",QUuid::createUuid().toString());
        if(!moment.contains("createdMs"))moment.insert("createdMs",double(QDateTime::currentMSecsSinceEpoch()));
        return change([=](QJsonObject &root){auto rows=root["moments"].toArray();bool replaced=false;for(int i=0;i<rows.size();i++)if(rows[i].toObject()["id"]==moment["id"]){rows.replace(i,moment);replaced=true;break;}if(!replaced)rows.append(moment);root.insert("moments",rows);});
    }
    bool deleteMoment(const QString &id){return change([=](QJsonObject &root){auto rows=root["moments"].toArray();for(int i=rows.size()-1;i>=0;i--)if(rows[i].toObject()["id"].toString()==id)rows.removeAt(i);root.insert("moments",rows);});}
    bool setTags(const QString &uri,const QStringList &tags){return change([=](QJsonObject &root){auto all=root["media"].toObject();auto m=all[uri].toObject();m.insert("uri",uri);m.insert("title",title(uri));QStringList clean;for(const QString &s:tags){QString t=s.trimmed();if(!t.isEmpty()&&!clean.contains(t,Qt::CaseInsensitive))clean.append(t);}m.insert("tags",QJsonArray::fromStringList(clean));all.insert(uri,m);root.insert("media",all);});}
    bool saveCollection(QJsonObject collection){if(collection["name"].toString().trimmed().isEmpty()){error=qtr("Give the collection a name.");return false;}if(collection["id"].toString().isEmpty())collection.insert("id",QUuid::createUuid().toString());return change([=](QJsonObject &root){auto a=root["collections"].toArray();a.append(collection);root.insert("collections",a);});}
    bool deleteCollection(const QString &id){return change([=](QJsonObject &root){auto a=root["collections"].toArray();for(int i=a.size()-1;i>=0;i--)if(a[i].toObject()["id"].toString()==id)a.removeAt(i);root.insert("collections",a);});}
    static bool matches(const QJsonObject &m,const QString &rule,const QString &value,qint64 now=QDateTime::currentMSecsSinceEpoch()) {
        const double duration=m["durationMs"].toDouble(),position=m["positionMs"].toDouble();
        if(rule=="unfinished")return position>0 && (duration<=0 || position<duration*.95);
        if(rule=="recent"){const double added=m["addedMs"].toDouble();return added>0 && now>=added && now-added<=value.toDouble()*86400000.;}
        if(rule=="duration")return duration>0 && duration<value.toDouble()*60000.;
        if(rule=="tag"){for(auto tag:m["tags"].toArray())if(tag.toString().compare(value,Qt::CaseInsensitive)==0)return true;return false;}
        return rule=="all";
    }
    QString markdown(const QString &uri=QString())const {
        QString text="# Coneic study notes\n\n";
        for(auto v:moments()){auto m=v.toObject();if(!uri.isEmpty()&&m["uri"].toString()!=uri)continue;
            QString heading=m["title"].toString();heading.replace('\n',' ');heading.replace('#',"\\#");
            text+="## "+heading+"\n\nTime: "+stamp(qint64(m["positionMs"].toDouble()));if(m["endMs"].toDouble()>m["positionMs"].toDouble())text+=" - "+stamp(qint64(m["endMs"].toDouble()));
            text+="\n\nSource: "+QUrl::fromEncoded(m["uri"].toString().toUtf8()).toDisplayString(QUrl::RemoveUserInfo)+"\n\n"+m["note"].toString()+"\n\n";
        }return text;
    }
};
#endif
