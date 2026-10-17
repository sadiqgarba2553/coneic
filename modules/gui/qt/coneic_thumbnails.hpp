/* Native VLC scene-filter thumbnails, isolated from the interactive player.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONEIC_THUMBNAILS_HPP
#define CONEIC_THUMBNAILS_HPP
#include <QApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QSaveFile>
#include <QTimer>
#include <QHash>
#include <QQueue>
#include <QSet>
#include <QDateTime>
#include <functional>

class ConeicThumbnails : public QObject
{
    struct Job {QString key,path;int seconds;bool priority;};
    using Callback=std::function<void(const QString&)>;
    QString cache;
    QQueue<Job> queue;
    QHash<QString,QList<Callback>> callbacks;
    QSet<QString> failed;
    QProcess *worker;
    QTimer *deadline;
    Job current;
    bool active=false;
    void diagnostic(const QByteArray &message)
    {
        const QString directory=QString::fromLocal8Bit(qgetenv("CONEIC_TEST_OUTPUT"));
        if(directory.isEmpty())return;
        QFile file(directory+"/thumbnail-workers.log");
        if(file.open(QIODevice::WriteOnly|QIODevice::Append))file.write(message);
    }
    void next()
    {
        if(active || queue.isEmpty())return;
        current=queue.dequeue();active=true;
        QStringList args={"--ignore-config","--no-media-library","--no-one-instance","--no-metadata-network-access","-I","dummy","--play-and-exit",
            "--avcodec-hw=none","--avcodec-threads=1","--vout=dummy","--aout=dummy","--no-audio","--no-spu","--no-sub-autodetect-file",
            "--video-filter=scene","--scene-format=png","--scene-width=320","--scene-ratio=1","--scene-replace",
            "--scene-prefix="+current.key,"--scene-path="+QDir::toNativeSeparators(cache+"/work"),"--start-time="+QString::number(current.seconds),
            "--stop-time="+QString::number(current.seconds+.3,'f',1),QDir::toNativeSeparators(current.path)};
#ifdef Q_OS_WIN
        args.prepend(QStringLiteral("--no-crashdump"));
#endif
        diagnostic("start: "+QCoreApplication::applicationFilePath().toUtf8()+"\n"+args.join(" | ").toUtf8()+"\n");
        worker->start(QCoreApplication::applicationFilePath(),args);
        deadline->start(8000);
    }
    void finish()
    {
        if(!active)return;
        diagnostic("finish: "+QByteArray::number(worker->exitCode())+" "+worker->errorString().toUtf8()+"\n");
        deadline->stop();
        const QString temporary=cache+"/work/"+current.key+".png";
        const QString target=cache+"/"+current.key+".png";
        QImage image(temporary);QString result;
        if(!image.isNull()) {
            QSaveFile output(target);
            if(output.open(QIODevice::WriteOnly) && image.save(&output,"PNG") && output.commit()) {
                result=target;
                if(qApp)qApp->setProperty("coneicThumbnailCompleted",qApp->property("coneicThumbnailCompleted").toInt()+1);
            }
        }
        // Only the hash-named intermediate file in our private work directory.
        QFile::remove(temporary);
        if(result.isEmpty())failed.insert(current.key);
        const auto listeners=callbacks.take(current.key);
        active=false;
        for(const Callback &callback:listeners)callback(result);
        QTimer::singleShot(0,this,[this](){next();});
    }
    explicit ConeicThumbnails(const QString &directory):QObject(qApp),cache(directory)
    {
        setObjectName("ConeicThumbnailService");QDir().mkpath(cache+"/work");
        worker=new QProcess(this);deadline=new QTimer(this);deadline->setSingleShot(true);
        QProcessEnvironment environment=QProcessEnvironment::systemEnvironment();
        environment.remove("CONEIC_TEST_OUTPUT");environment.remove("CONEIC_TEST_MEDIA");
        worker->setProcessEnvironment(environment);
        connect(worker,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,[this](){finish();});
        connect(worker,QOverload<QProcess::ProcessError>::of(&QProcess::error),this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)finish();});
        connect(worker,&QProcess::readyReadStandardError,this,[this](){diagnostic(worker->readAllStandardError());});
        connect(worker,&QProcess::readyReadStandardOutput,this,[this](){worker->readAllStandardOutput();});
        connect(deadline,&QTimer::timeout,this,[this](){worker->kill();});
    }
public:
    static ConeicThumbnails *instance(const QString &directory)
    {
        QObject *existing=qApp->findChild<QObject*>(QStringLiteral("ConeicThumbnailService"));
        return existing?static_cast<ConeicThumbnails*>(existing):new ConeicThumbnails(directory);
    }
    void request(const QString &path,int seconds,Callback callback,bool priority=false)
    {
        QFileInfo file(path);
        if(!file.isFile()){callback(QString());return;}
        seconds=qMax(0,seconds);
        const QByteArray identity=(file.absoluteFilePath()+"|"+QString::number(file.size())+"|"+QString::number(file.lastModified().toMSecsSinceEpoch())+"|"+QString::number(seconds)).toUtf8();
        const QString key=QString::fromLatin1(QCryptographicHash::hash(identity,QCryptographicHash::Sha256).toHex());
        const QString target=cache+"/"+key+".png";
        if(QFileInfo::exists(target)) {
            qApp->setProperty("coneicThumbnailCacheHits",qApp->property("coneicThumbnailCacheHits").toInt()+1);
            QTimer::singleShot(0,this,[callback,target](){callback(target);});return;
        }
        if(failed.contains(key)){callback(QString());return;}
        if(callbacks.contains(key)){callbacks[key].append(callback);return;}
        if(priority) {
            QQueue<Job> retained;
            while(!queue.isEmpty()) {
                Job old=queue.dequeue();
                if(old.priority){for(const Callback &listener:callbacks.take(old.key))listener(QString());}
                else retained.enqueue(old);
            }
            queue=retained;
            if(queue.size()>=64){Job old=queue.takeLast();for(const Callback &listener:callbacks.take(old.key))listener(QString());}
        }
        if(queue.size()>=64){callback(QString());return;}
        callbacks[key].append(callback);
        Job job={key,file.absoluteFilePath(),seconds,priority};
        if(priority)queue.prepend(job);else queue.enqueue(job);
        next();
    }
    ~ConeicThumbnails() override
    {
        active=false;deadline->stop();worker->disconnect(this);callbacks.clear();queue.clear();
        if(worker->state()!=QProcess::NotRunning){worker->kill();worker->waitForFinished(1000);}
    }
};
#endif
