/* Coneic library home; native VLC playback and dialogs.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Design: Juxtopposed, CC BY 4.0; see pixmaps/coneic-reference/ATTRIBUTION.md.
 */
#ifndef CONEIC_HOME_HPP
#define CONEIC_HOME_HPP
#include "recents.hpp"
#include "dialogs_provider.hpp"
#include "coneic_thumbnails.hpp"
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
#include <functional>
#include <memory>

class ConeicHome : public QWidget
{
    intf_thread_t *intf;
    QListWidget *navigation,*cards;
    QLabel *title,*section,*empty;
    QLineEdit *search;
    QStringList libraryFiles;
    std::function<void()> browse;
    static bool isAudio(const QString &suffix) {static const QStringList formats=QStringLiteral(EXTENSIONS_AUDIO).toLower().split(';');return formats.contains("*."+suffix.toLower());}
    static bool isPlaylist(const QString &suffix) {static const QStringList formats=QStringLiteral(EXTENSIONS_PLAYLIST).toLower().split(';');return formats.contains("*."+suffix.toLower());}
    static bool isVideo(const QString &suffix) {static const QStringList formats=QStringLiteral(EXTENSIONS_VIDEO).toLower().split(';');return formats.contains("*."+suffix.toLower());}
    void refresh()
    {
        cards->clear();
        const int mode=qMax(0,navigation->currentRow());
        title->setText(navigation->item(mode)->text());
        section->setText(qtr("Your media"));section->setVisible(mode!=0);
        QStringList files=RecentsMRL::getInstance(intf)->recentList();
        QSet<QString> seen=QSet<QString>::fromList(files);
        if(mode!=1)for(const QString &f:libraryFiles)if(!seen.contains(f)){seen.insert(f);files.append(f);}
        if(!search->text().isEmpty()) {
            QStringList filtered;
            for(const QString &mrl:files){const QUrl url=QUrl::fromEncoded(mrl.toUtf8());QString label=QFileInfo(url.path()).completeBaseName();if(label.isEmpty())label=url.host();if(label.contains(search->text(),Qt::CaseInsensitive))filtered.append(mrl);}
            files=filtered;
        }
        QMap<QString,QString> headings;
        if(mode==0) {
            QList<QStringList> groups;for(int i=0;i<6;i++)groups.append(QStringList());
            for(const QString &mrl:files) {
                const QUrl url=QUrl::fromEncoded(mrl.toUtf8());
                const QString suffix=QFileInfo(url.path()).suffix();
                if(isVideo(suffix) && RecentsMRL::getInstance(intf)->time(mrl)>0)groups[0].append(mrl);
                const int group=!url.isLocalFile()?4:(isPlaylist(suffix)?1:(isVideo(suffix)?2:(isAudio(suffix)?3:5)));
                groups[group].append(mrl);
            }
            const QStringList titles={qtr("Continue Watching"),qtr("Playlists"),qtr("New Videos"),qtr("New Music"),qtr("Network Streams"),qtr("Recent Media")};
            files.clear();
            for(int i=0;i<groups.size();i++)if(!groups[i].isEmpty()) {
                const QString marker=QStringLiteral("coneic-section:")+QString::number(i);
                files.append(marker);headings[marker]=titles[i];files.append(groups[i]);
            }
        }
        for(const QString &mrl:files) {
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
            if(!iterator->hasNext()){timer->stop();timer->deleteLater();refresh();}
        });timer->start();
    }
protected:
    void showEvent(QShowEvent *event) override {QWidget::showEvent(event);refresh();}
    bool eventFilter(QObject *object,QEvent *event) override {
        if(object==cards->viewport() && event->type()==QEvent::Resize)
            for(int i=0;i<cards->count();i++)if(cards->item(i)->data(Qt::UserRole+1).toBool())cards->item(i)->setSizeHint(QSize(qMax(200,cards->viewport()->width()-2*cards->spacing()),32));
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
        const QStringList names={qtr("Home"),qtr("Recent"),qtr("Videos"),qtr("Music"),qtr("Playlists"),qtr("Network Streams")};
        const QStringList icons={"home","recent","videos","music","playlists","network"};
        for(int i=0;i<names.size();i++){auto item=new QListWidgetItem(QIcon(":/coneic/"+icons[i]+".svg"),names[i],navigation);item->setSizeHint(QSize(170,36));}
        side->addWidget(navigation,1);
        auto browseButton=new QPushButton(QIcon(":/coneic/folder.svg"),qtr("Browse"),sidebar);
        connect(browseButton,&QPushButton::clicked,this,[this](){browse();});side->addWidget(browseButton);
        auto settings=new QPushButton(QIcon(":/coneic/menu-settings.svg"),qtr("Settings"),sidebar);
        connect(settings,&QPushButton::clicked,DialogsProvider::getInstance(intf),&DialogsProvider::prefsDialog);side->addWidget(settings);
        layout->addWidget(sidebar);
        QWidget *main=new QWidget(this);QVBoxLayout *content=new QVBoxLayout(main);content->setContentsMargins(28,22,28,18);content->setSpacing(16);
        QHBoxLayout *top=new QHBoxLayout;
        title=new QLabel(qtr("Home"),main);title->setObjectName("ConeicHomeTitle");top->addWidget(title,1);
        search=new QLineEdit(main);search->setPlaceholderText(qtr("Search your media"));search->setAccessibleName(qtr("Search your media"));search->setMaximumWidth(250);search->setClearButtonEnabled(true);top->addWidget(search);content->addLayout(top);
        QHBoxLayout *actions=new QHBoxLayout;
        auto file=new QPushButton(QIcon(":/coneic/menu-file.svg"),qtr("Open File"),main);file->setObjectName("ConeicOpenFile");
        connect(file,&QPushButton::clicked,DialogsProvider::getInstance(intf),&DialogsProvider::simpleOpenDialog);actions->addWidget(file);
        auto folder=new QPushButton(QIcon(":/coneic/menu-folder.svg"),qtr("Add Folder"),main);connect(folder,&QPushButton::clicked,this,[this](){addFolder();});actions->addWidget(folder);
        auto network=new QPushButton(QIcon(":/coneic/menu-network.svg"),qtr("Network Stream"),main);connect(network,&QPushButton::clicked,DialogsProvider::getInstance(intf),&DialogsProvider::openNetDialog);actions->addWidget(network);actions->addStretch();content->addLayout(actions);
        section=new QLabel(main);section->setObjectName("ConeicSectionTitle");content->addWidget(section);
        cards=new QListWidget(main);cards->setObjectName("ConeicMediaCards");cards->setViewMode(QListView::IconMode);cards->setResizeMode(QListView::Adjust);cards->setMovement(QListView::Static);cards->setWordWrap(true);cards->setIconSize(QSize(170,96));cards->setSpacing(12);cards->setSelectionMode(QAbstractItemView::SingleSelection);
        cards->viewport()->installEventFilter(this);
        connect(cards,&QListWidget::itemActivated,this,[this](QListWidgetItem *item){const QString mrl=item->data(Qt::UserRole).toString();if(!mrl.isEmpty())RecentsMRL::getInstance(intf)->playMRL(mrl);});content->addWidget(cards,1);
        empty=new QLabel(qtr("Your library starts here.\nOpen a file or add a media folder to continue."),main);empty->setObjectName("ConeicLibraryEmpty");empty->setWordWrap(true);empty->setAlignment(Qt::AlignCenter);content->addWidget(empty,1);
        layout->addWidget(main,1);navigation->setCurrentRow(0);
        connect(navigation,&QListWidget::currentRowChanged,this,[this](){refresh();});connect(search,&QLineEdit::textChanged,this,[this](){refresh();});
        QSettings &settingsStore=*intf->p_sys->mainSettings;for(const QString &folder:settingsStore.value("Coneic/library-folders").toStringList())indexFolder(folder);
        refresh();
    }
};
#endif
