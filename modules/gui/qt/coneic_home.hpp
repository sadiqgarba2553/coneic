/* Coneic library home; native VLC playback and dialogs.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Design: Juxtopposed, CC BY 4.0; see pixmaps/coneic-reference/ATTRIBUTION.md.
 */
#ifndef CONEIC_HOME_HPP
#define CONEIC_HOME_HPP
#include "recents.hpp"
#include "dialogs_provider.hpp"
#include "coneic_thumbnails.hpp"
#include "coneic_personal.hpp"
#include "coneic_home_options.hpp"
#include <QListWidget>
#include <QFileDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QSettings>
#include <QPointer>
#include <QMap>
#include <QSet>
#include <QLineEdit>
#include <QMenu>
#include <functional>
#include <memory>

class ConeicHome : public QWidget
{
    intf_thread_t *intf;
    QListWidget *navigation=nullptr,*cards=nullptr;
    QLabel *title,*section,*empty;
    QLineEdit *search;
    QStringList libraryFiles;
    std::function<void()> browse;
    static bool isAudio(const QString &suffix) {static const QStringList formats=QStringLiteral(EXTENSIONS_AUDIO).toLower().split(';');return formats.contains("*."+suffix.toLower());}
    static bool isPlaylist(const QString &suffix) {static const QStringList formats=QStringLiteral(EXTENSIONS_PLAYLIST).toLower().split(';');return formats.contains("*."+suffix.toLower());}
    static bool isVideo(const QString &suffix) {static const QStringList formats=QStringLiteral(EXTENSIONS_VIDEO).toLower().split(';');return formats.contains("*."+suffix.toLower());}
    void layoutCards() {
        if(!cards)return;const int available=qMax(180,cards->viewport()->width()-2*cards->spacing());const int columns=qMax(1,(available+cards->spacing())/(210+cards->spacing()));const int cell=qMin(280,(available-(columns-1)*cards->spacing())/columns);const int imageHeight=(cell-16)*9/16;cards->setIconSize(QSize(cell-16,imageHeight));
        for(int i=0;i<cards->count();i++){auto item=cards->item(i);const bool heading=item->data(Qt::UserRole+1).toBool();const QSize size=heading?QSize(available,40):QSize(cell,imageHeight+68);if(item->sizeHint()!=size)item->setSizeHint(size);if(!heading){auto lines=item->data(Qt::UserRole+4).toString().split('\n');for(QString &line:lines)line=cards->fontMetrics().elidedText(line,Qt::ElideRight,cell-24);const auto caption=lines.join("\n");if(item->text()!=caption)item->setText(caption);}}
    }
    void refresh()
    {
        cards->clear();
        const int mode=qMax(0,navigation->currentRow());
        title->setText(navigation->item(mode)->text());
        section->setText(qtr("Your media"));section->setVisible(mode!=0);
        auto personal=ConeicPersonalStore::instance(intf);QStringList files;for(const QString &uri:RecentsMRL::getInstance(intf)->recentList())files.append(personal->resolve(uri));
        QSet<QString> seen=QSet<QString>::fromList(files);
        if(mode!=1)for(const QString &f:libraryFiles){const QString resolved=personal->resolve(f);if(!seen.contains(resolved)){seen.insert(resolved);files.append(resolved);}}
        if(mode!=1)for(const QString &uri:personal->media().keys())if(!seen.contains(uri)){seen.insert(uri);files.append(uri);}
        if(!search->text().isEmpty()) {
            QStringList filtered;
            for(const QString &mrl:files){const QUrl url=QUrl::fromEncoded(mrl.toUtf8());QString label=QFileInfo(url.path()).completeBaseName();if(label.isEmpty())label=url.host();if(label.contains(search->text(),Qt::CaseInsensitive))filtered.append(mrl);}
            files=filtered;
        }
        QMap<QString,QString> headings;QMap<QString,QJsonObject> moments,collections;
        if(mode==0) {
            QList<QStringList> groups;for(int i=0;i<8;i++)groups.append(QStringList());
            for(const QString &mrl:files) {
                const QUrl url=QUrl::fromEncoded(mrl.toUtf8());
                const QString suffix=QFileInfo(url.path()).suffix();
                if(isVideo(suffix) && RecentsMRL::getInstance(intf)->time(mrl)>0)groups[0].append(mrl);
                const int group=!url.isLocalFile()?4:(isPlaylist(suffix)?1:(isVideo(suffix)?2:(isAudio(suffix)?3:5)));
                groups[group].append(mrl);
            }
            const auto saved=personal->moments();for(int i=saved.size()-1;i>=0&&groups[6].size()<6;i--){auto m=saved.at(i).toObject();if(!search->text().isEmpty()&&!m.value("title").toString().contains(search->text(),Qt::CaseInsensitive))continue;const QString marker="coneic-moment:"+m.value("id").toString();moments.insert(marker,m);groups[6].append(marker);}
            QJsonArray choices{QJsonObject{{"id","all"},{"name",qtr("All media")}},QJsonObject{{"id","unfinished"},{"name",qtr("Unfinished")}},QJsonObject{{"id","week"},{"name",qtr("Added this week")}},QJsonObject{{"id","short"},{"name",qtr("Under 20 minutes")}}};for(auto c:personal->collections())choices.append(c);
            for(auto pin:personal->home().value("pinnedCollections").toArray())for(auto c:choices)if(c.toObject().value("id")==pin){const QString marker="coneic-collection:"+pin.toString();collections.insert(marker,c.toObject());groups[7].append(marker);break;}
            const QStringList titles=ConeicHomeOptions::names();
            files.clear();
            QStringList order,hidden;for(auto id:personal->home().value("sections").toArray())if(ConeicHomeOptions::defaults().contains(id.toString())&&!order.contains(id.toString()))order.append(id.toString());for(const auto &id:ConeicHomeOptions::defaults())if(!order.contains(id))order.append(id);for(auto id:personal->home().value("hidden").toArray())hidden.append(id.toString());
            for(const QString &id:order){int i=id.toInt();if(!hidden.contains(id)&&!groups[i].isEmpty()) {
                const QString marker=QStringLiteral("coneic-section:")+QString::number(i);
                files.append(marker);headings[marker]=titles[i];files.append(groups[i]);
            }}
        }
        for(const QString &mrl:files) {
            if(moments.contains(mrl)||collections.contains(mrl)) {
                const bool moment=moments.contains(mrl);const auto m=moment?moments.value(mrl):collections.value(mrl);
                const QString label=moment?m.value("title").toString()+"\n"+ConeicPersonalStore::stamp(qint64(m.value("positionMs").toDouble())):m.value("name").toString();
                auto item=new QListWidgetItem(QIcon(moment?":/coneic/bookmarks.svg":":/coneic/playlists.svg"),label,cards);item->setData(Qt::UserRole,mrl);item->setData(Qt::UserRole+2,m);item->setData(Qt::UserRole+3,moment?"moment":"collection");item->setData(Qt::UserRole+4,label);item->setToolTip(label);item->setSizeHint(QSize(192,150));
                if(moment){const QUrl url=QUrl::fromEncoded(m.value("uri").toString().toUtf8());if(url.isLocalFile()&&isVideo(QFileInfo(url.path()).suffix())){QPointer<ConeicHome> guard(this);ConeicThumbnails::instance(QFileInfo(intf->p_sys->mainSettings->fileName()).absolutePath()+"/thumbnails")->request(url.toLocalFile(),int(m.value("positionMs").toDouble()/1000),[guard,mrl](const QString &path){if(guard&&!path.isEmpty())for(int i=0;i<guard->cards->count();i++)if(guard->cards->item(i)->data(Qt::UserRole).toString()==mrl)guard->cards->item(i)->setIcon(QIcon(path));});}}
                continue;
            }
            if(headings.contains(mrl)) {
                auto heading=new QListWidgetItem(headings[mrl],cards);
                heading->setData(Qt::UserRole+1,true);heading->setFlags(Qt::ItemIsEnabled);
                heading->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
                QFont font=cards->font();font.setWeight(QFont::DemiBold);heading->setFont(font);
                heading->setSizeHint(QSize(qMax(200,cards->viewport()->width()-2*cards->spacing()),32));
                continue;
            }
            QUrl url=QUrl::fromEncoded(mrl.toUtf8());
            QFileInfo file(url.isLocalFile()?url.toLocalFile():url.path());
            const QString suffix=file.suffix();
            const bool network=!url.isLocalFile() && !url.scheme().isEmpty();
            if(mode==2&&!isVideo(suffix))continue;
            if(mode==3&&!isAudio(suffix))continue;
            if(mode==4&&!isPlaylist(suffix))continue;
            if(mode==5&&!network)continue;
            QString label=file.completeBaseName();if(label.isEmpty())label=url.host();
            if(!search->text().isEmpty()&&!label.contains(search->text(),Qt::CaseInsensitive))continue;
            const QString kind=isAudio(suffix)?"music":(network?"network":(isPlaylist(suffix)?"playlists":"videos"));
            QIcon icon(":/coneic/"+kind+".svg");
            bool hasCover=false;
            if(url.isLocalFile()) {
                for(const QString &art:QStringList({"cover.jpg","folder.jpg","cover.png"})) {
                    const QString candidate=file.dir().absoluteFilePath(art);
                    if(QFileInfo::exists(candidate)){icon=QIcon(candidate);hasCover=true;break;}
                }
            }
            int resume=RecentsMRL::getInstance(intf)->time(mrl);
            if(resume>0) label+=QString("\n%1 %2:%3").arg(qtr("Resume")).arg(resume/60000).arg((resume/1000)%60,2,10,QChar('0'));
            auto item=new QListWidgetItem(icon,label,cards);
            item->setData(Qt::UserRole+4,label);
            item->setData(Qt::UserRole,mrl);item->setToolTip(QUrl::fromEncoded(mrl.toUtf8()).toDisplayString(QUrl::RemoveUserInfo));
            item->setSizeHint(QSize(192,150));
            if(!hasCover && url.isLocalFile() && isVideo(suffix) && cards->count()<=32) {
                const QString cache=QFileInfo(intf->p_sys->mainSettings->fileName()).absolutePath()+"/thumbnails";
                QPointer<ConeicHome> guard(this);
                ConeicThumbnails::instance(cache)->request(url.toLocalFile(),1,[guard,mrl](const QString &image){
                    if(!guard || image.isEmpty())return;
                    for(int i=0;i<guard->cards->count();i++) {
                        QListWidgetItem *item=guard->cards->item(i);
                        if(item->data(Qt::UserRole).toString()==mrl)item->setIcon(QIcon(image));
                    }
                });
            }
        }
        empty->setVisible(cards->count()==0);
        empty->setText(search->text().isEmpty()?qtr("Your library starts here.\nOpen a file or add a media folder to continue."):qtr("No media matches your search."));
        cards->setVisible(cards->count()!=0);
        layoutCards();
    }
    void addFolder()
    {
        const QString folder=QFileDialog::getExistingDirectory(this,qtr("Add a media folder"));
        if(folder.isEmpty())return;
        QSettings &settings=*intf->p_sys->mainSettings;QStringList folders=settings.value("Coneic/library-folders").toStringList();
        if(!folders.contains(folder)){folders.append(folder);settings.setValue("Coneic/library-folders",folders);}
        indexFolder(folder);
    }
    void indexFolder(const QString &folder)
    {
        auto iterator=std::make_shared<QDirIterator>(folder,QDir::Files|QDir::Readable,QDirIterator::Subdirectories);
        auto timer=new QTimer(this);timer->setInterval(16);
        connect(timer,&QTimer::timeout,this,[this,iterator,timer](){
            // Yield frequently so a large library never locks navigation or playback.
            for(int i=0;i<100 && iterator->hasNext();i++) {
                const QString file=iterator->next();const QString suffix=QFileInfo(file).suffix();
                if(isAudio(suffix)||isVideo(suffix)||isPlaylist(suffix)) {
                    const QString mrl=QString::fromUtf8(QUrl::fromLocalFile(file).toEncoded());
                    if(!libraryFiles.contains(mrl))libraryFiles.append(mrl);
                }
            }
            if(!iterator->hasNext()){timer->stop();timer->deleteLater();ConeicPersonalStore::instance(intf)->catalog(libraryFiles);refresh();}
        });timer->start();
    }
protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if(auto sidebar=findChild<QWidget*>(QStringLiteral("ConeicLibrarySidebar")))sidebar->setFixedWidth(width()<850?178:210);
    }
    void showEvent(QShowEvent *event) override {QWidget::showEvent(event);refresh();}
    bool eventFilter(QObject *object,QEvent *event) override {
        if(cards && object==cards->viewport() && event->type()==QEvent::Resize)layoutCards();
        return QWidget::eventFilter(object,event);
    }
public:
    ConeicHome(intf_thread_t *p,std::function<void()> showLibrary,QWidget *parent=nullptr):QWidget(parent),intf(p),browse(showLibrary)
    {
        setObjectName("ConeicHome");
        QHBoxLayout *layout=new QHBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
        QWidget *sidebar=new QWidget(this);sidebar->setObjectName("ConeicLibrarySidebar");sidebar->setFixedWidth(210);
        QVBoxLayout *side=new QVBoxLayout(sidebar);side->setContentsMargins(10,18,10,12);
        QLabel *brand=new QLabel("Coneic",sidebar);brand->setObjectName("ConeicBrand");side->addWidget(brand);
        navigation=new QListWidget(sidebar);navigation->setObjectName("ConeicLibraryNavigation");navigation->setFrameShape(QFrame::NoFrame);
        const QStringList names={qtr("Home"),qtr("Recent"),qtr("Videos"),qtr("Music"),qtr("Playlists"),qtr("Streams")};
        const QStringList icons={"home","recent","videos","music","playlists","network"};
        for(int i=0;i<names.size();i++){auto item=new QListWidgetItem(QIcon(":/coneic/"+icons[i]+".svg"),names[i],navigation);item->setSizeHint(QSize(170,36));}
        side->addWidget(navigation,1);
        auto personal=new QPushButton(QIcon(":/coneic/bookmarks.svg"),qtr("Your space"),sidebar);connect(personal,&QPushButton::clicked,this,[this](){ConeicPersonalDialog::open(intf,window());});side->addWidget(personal);
        auto browseButton=new QPushButton(QIcon(":/coneic/folder.svg"),qtr("Browse"),sidebar);
        connect(browseButton,&QPushButton::clicked,this,[this](){browse();});side->addWidget(browseButton);
        auto settings=new QPushButton(QIcon(":/coneic/menu-settings.svg"),qtr("Settings"),sidebar);
        connect(settings,&QPushButton::clicked,DialogsProvider::getInstance(intf),&DialogsProvider::prefsDialog);side->addWidget(settings);
        layout->addWidget(sidebar);
        QWidget *main=new QWidget(this);QVBoxLayout *content=new QVBoxLayout(main);content->setContentsMargins(28,22,28,18);content->setSpacing(16);
        QHBoxLayout *top=new QHBoxLayout;
        title=new QLabel(qtr("Home"),main);title->setObjectName("ConeicHomeTitle");top->addWidget(title,1);
        search=new QLineEdit(main);search->setPlaceholderText(qtr("Search media"));search->setAccessibleName(qtr("Search your media"));search->setMinimumWidth(140);search->setMaximumWidth(260);search->setClearButtonEnabled(true);top->addWidget(search);content->addLayout(top);
        QHBoxLayout *actions=new QHBoxLayout;
        auto file=new QPushButton(QIcon(":/coneic/menu-file.svg"),qtr("Open File"),main);file->setObjectName("ConeicOpenFile");
        connect(file,&QPushButton::clicked,DialogsProvider::getInstance(intf),&DialogsProvider::simpleOpenDialog);actions->addWidget(file);
        auto add=new QPushButton(qtr("Add media"),main);auto addMenu=new QMenu(add);
        connect(addMenu->addAction(QIcon(":/coneic/menu-folder.svg"),qtr("Media folder")),&QAction::triggered,this,[this](){addFolder();});
        connect(addMenu->addAction(QIcon(":/coneic/menu-network.svg"),qtr("Network stream")),&QAction::triggered,DialogsProvider::getInstance(intf),&DialogsProvider::openNetDialog);
        add->setMenu(addMenu);actions->addWidget(add);actions->addStretch();auto customise=new QToolButton(main);customise->setIcon(QIcon(":/coneic/menu-customize.svg"));customise->setToolTip(qtr("Customise Home"));customise->setAccessibleName(qtr("Customise Home"));customise->setFixedSize(36,36);customise->setObjectName("ConeicCustomiseHome");connect(customise,&QToolButton::clicked,this,[this](){ConeicHomeOptions::open(intf,window());});actions->addWidget(customise);content->addLayout(actions);
        section=new QLabel(main);section->setObjectName("ConeicSectionTitle");content->addWidget(section);
        cards=new QListWidget(main);cards->setObjectName("ConeicMediaCards");cards->setViewMode(QListView::IconMode);cards->setResizeMode(QListView::Adjust);cards->setMovement(QListView::Static);cards->setWordWrap(true);cards->setIconSize(QSize(170,96));cards->setSpacing(12);cards->setSelectionMode(QAbstractItemView::SingleSelection);
        cards->viewport()->installEventFilter(this);
        connect(cards,&QListWidget::itemActivated,this,[this](QListWidgetItem *item){if(item->data(Qt::UserRole+3).toString()=="moment"){ConeicPersonalDialog::open(intf,window())->playSaved(item->data(Qt::UserRole+2).toJsonObject());return;}if(item->data(Qt::UserRole+3).toString()=="collection"){ConeicPersonalDialog::open(intf,window(),2)->selectCollection(item->data(Qt::UserRole+2).toJsonObject().value("id").toString());return;}const QString mrl=item->data(Qt::UserRole).toString();if(!mrl.isEmpty()){const auto url=QUrl::fromEncoded(mrl.toUtf8());if(url.isLocalFile()&&!QFileInfo::exists(url.toLocalFile()))ConeicLibraryTools::locate(intf,window(),mrl);else RecentsMRL::getInstance(intf)->playMRL(mrl);}});content->addWidget(cards,1);
        empty=new QLabel(qtr("Your library starts here.\nOpen a file or add a media folder to continue."),main);empty->setObjectName("ConeicLibraryEmpty");empty->setWordWrap(true);empty->setAlignment(Qt::AlignCenter);content->addWidget(empty,1);
        layout->addWidget(main,1);navigation->setCurrentRow(0);
        connect(navigation,&QListWidget::currentRowChanged,this,[this](){refresh();});connect(search,&QLineEdit::textChanged,this,[this](){refresh();});
        QSettings &settingsStore=*intf->p_sys->mainSettings;for(const QString &folder:settingsStore.value("Coneic/library-folders").toStringList())indexFolder(folder);
        ConeicPersonalStore::instance(intf)->catalog(RecentsMRL::getInstance(intf)->recentList());
        ConeicPersonalStore::instance(intf)->watch(this,[this](){if(isVisible())refresh();});
        refresh();
    }
};
#endif
