/* Regression checks for local library tools. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_LIBRARY_SMOKE_HPP
#define CONEIC_LIBRARY_SMOKE_HPP
#include "coneic_personal_store.hpp"
#include <QTemporaryDir>
namespace ConeicLibrarySmoke {
inline bool run(const QString &directory){QTemporaryDir temp;QJsonArray checks;auto check=[&](const QString &name,bool pass){checks.append(QJsonObject{{"name",name},{"passed",pass}});};
    const QString old="file:///C:/Missing/lecture.mp4";const QString file=temp.path()+"/relocated lecture.mp4";QFile mediaFile(file);mediaFile.open(QIODevice::WriteOnly);mediaFile.write("synthetic media reference");mediaFile.close();const QString uri=QString::fromUtf8(QUrl::fromLocalFile(file).toEncoded());
    ConeicPersonalStore source(nullptr,temp.path()+"/source.json");source.catalog({old});source.setTags(old,{"Learning"});source.saveMoment({{"id","note"},{"uri",old},{"title","Lecture"},{"note","Keep this note"},{"positionMs",2000.},{"endMs",4000.}});
    check("Relinking rejects unavailable replacement",!source.relink(old,"file:///C:/Missing/other.mp4"));
    check("Relinking moves saved notes and tags",source.relink(old,uri)&&source.moments().at(0).toObject().value("uri").toString()==uri&&source.media().value(uri).toObject().value("tags").toArray().size()==1&&!source.media().contains(old));
    check("Old references resolve to the selected replacement",source.resolve(old)==uri);
    QFile original(file);original.open(QIODevice::ReadOnly);check("Relinking never changes the media file",original.readAll()=="synthetic media reference");original.close();
    const QJsonObject home{{"sections",QJsonArray({"6","2","0"})},{"hidden",QJsonArray({"3"})},{"pinnedCollections",QJsonArray({"unfinished"})}};check("Home choices save",source.saveHome(home));
    ConeicPersonalStore restarted(nullptr,temp.path()+"/source.json");check("Home order, visibility and pins survive restart",restarted.home()==home);
    const QString backup=temp.path()+"/backup.json";check("Portable backup exports",source.exportBackup(backup));
    ConeicPersonalStore destination(nullptr,temp.path()+"/destination.json");destination.catalog({"file:///C:/Existing/other.mp4"});destination.saveMoment({{"id","note"},{"uri","file:///C:/Existing/other.mp4"},{"title","Existing"},{"note","Keep existing"},{"positionMs",1000.}});
    check("Restore merges and retains identifier conflicts separately",destination.importBackup(backup)&&destination.moments().size()==2&&destination.media().size()==2);
    check("Restore keeps Home layout and tags",destination.home()==home&&destination.media().value(uri).toObject().value("tags").toArray().size()==1);
    check("Restoring the same backup is idempotent",destination.importBackup(backup)&&destination.moments().size()==2);
    check("Restore creates recovery snapshots",QDir(temp.path()).entryList({"destination.json.before-restore-*.json"},QDir::Files).size()>=1);
    const auto before=destination.moments();QFile invalid(temp.path()+"/invalid.json");invalid.open(QIODevice::WriteOnly);invalid.write("{\"format\":\"coneic-library-backup\",\"version\":99}");invalid.close();check("Invalid backups leave saved notes unchanged",!destination.importBackup(invalid.fileName())&&destination.moments()==before);
    check("Backup cannot overwrite active library file",!source.exportBackup(temp.path()+"/source.json"));
    ConeicPersonalStore again(nullptr,temp.path()+"/destination.json");check("Merged library survives restart",again.moments().size()==2&&again.home()==home);
    bool pass=true;for(auto v:checks)pass &= v.toObject().value("passed").toBool();QFile report(directory+"/library-tools-results.json");if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(QJsonObject{{"passed",pass},{"checks",checks}}).toJson());return pass;
}
}
#endif
