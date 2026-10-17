/* Searchable native actions and personal library. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_COMMAND_PALETTE_HPP
#define CONEIC_COMMAND_PALETTE_HPP
#include "coneic_personal.hpp"
#include "coneic_library_tools.hpp"
#include "coneic_home_options.hpp"
#include "actions_manager.hpp"
#include "dialogs_provider.hpp"
#include <vlc_modules.h>
#include <vlc_plugin.h>
#include <QKeyEvent>
#include <QAction>
#include <QSet>
#include <QShortcut>
class ConeicCommandPalette : public QDialog
{
    struct Entry {QString name,kind,key;std::function<void()> run;};
    QList<Entry> entries;QLineEdit *query;QListWidget *results;QLabel *hint;
    void filter(){results->clear();const auto words=query->text().toLower().split(' ',QString::SkipEmptyParts);int matches=0;
        for(int i=0;i<entries.size();i++){const auto &e=entries[i];const QString text=(e.name+" "+e.kind+" "+e.key).toLower();bool match=true;for(const auto &word:words)match &= text.contains(word);if(!match)continue;matches++;if(results->count()>=80)continue;auto item=new QListWidgetItem(e.name+"\n"+e.kind+(e.key.isEmpty()?QString():"  ·  "+e.key),results);item->setData(Qt::UserRole,i);item->setSizeHint(QSize(240,58));}if(results->count())results->setCurrentRow(0);hint->setText(matches?QString::number(matches)+qtr(" matches · Enter to run · Esc to close"):qtr("No matches. Search an action, setting, file or moment."));resize(width(),qMin(QGuiApplication::primaryScreen()->availableGeometry().height()-64,qBound(220,results->count()*64+120,520)));}
    void run(){if(!results->currentItem())return;const auto action=entries.at(results->currentItem()->data(Qt::UserRole).toInt()).run;accept();action();}
    void add(const QString &name,const QString &kind,std::function<void()> fn,const QString &key=QString()){entries.append({name,kind,key,fn});}
    static QString nativeKey(intf_thread_t *p,const char *name){char *raw=var_InheritString(p,name);QString key=qfu(raw);free(raw);return key.replace('\t',QStringLiteral(" / "));}
protected:
    bool eventFilter(QObject *object,QEvent *event)override {
        if(object==query&&event->type()==QEvent::KeyPress){auto key=static_cast<QKeyEvent*>(event);if(key->key()==Qt::Key_Down||key->key()==Qt::Key_Up){results->setCurrentRow(qBound(0,results->currentRow()+(key->key()==Qt::Key_Down?1:-1),qMax(0,results->count()-1)));return true;}}return QDialog::eventFilter(object,event);
    }
public:
    explicit ConeicCommandPalette(intf_thread_t *p,QWidget *parent):QDialog(parent){
        setObjectName("ConeicCommandPalette");setWindowTitle(qtr("Command palette"));setAttribute(Qt::WA_DeleteOnClose);resize(700,480);setMinimumSize(440,200);
        auto layout=new QVBoxLayout(this);layout->setContentsMargins(20,20,20,20);layout->setSpacing(12);query=new QLineEdit(this);query->setObjectName("ConeicCommandQuery");query->setPlaceholderText(qtr("Search commands, settings, media and moments"));query->setAccessibleName(qtr("Search commands"));layout->addWidget(query);results=new QListWidget(this);results->setObjectName("ConeicCommandResults");results->setMinimumSize(0,80);results->setSpacing(4);layout->addWidget(results,1);hint=new QLabel(this);hint->setWordWrap(true);layout->addWidget(hint);
        const QString action=qtr("Action");auto manager=ActionsManager::getInstance(p);
        add(qtr("Play or pause"),action,[manager](){manager->doAction(PLAY_ACTION);},nativeKey(p,"key-play-pause"));add(qtr("Toggle fullscreen"),action,[manager](){manager->doAction(FULLSCREEN_ACTION);},nativeKey(p,"key-toggle-fullscreen"));
        add(qtr("Stop playback"),action,[manager](){manager->doAction(STOP_ACTION);},nativeKey(p,"key-stop"));add(qtr("Skip forward"),action,[manager](){manager->doAction(SKIP_FW_ACTION);});add(qtr("Skip backward"),action,[manager](){manager->doAction(SKIP_BACK_ACTION);});add(qtr("Mute or unmute"),action,[manager](){manager->toggleMuteAudio();},nativeKey(p,"key-vol-mute"));add(qtr("Show playlist"),action,[manager](){manager->doAction(PLAYLIST_ACTION);});
        add(qtr("Keyboard shortcuts"),action,[=](){QString text;for(auto a:parent->findChildren<QAction*>())if(QStringList({qtr("Command palette"),qtr("Save current moment"),qtr("Study notes"),qtr("Library / player")}).contains(a->text()))text+=a->text()+"  —  "+(a->shortcut().isEmpty()?qtr("Available from the menu"):a->shortcut().toString(QKeySequence::NativeText))+"\n";text+="\n"+qtr("Play / pause")+"  —  "+nativeKey(p,"key-play-pause")+"\n"+qtr("Fullscreen")+"  —  "+nativeKey(p,"key-toggle-fullscreen")+"\n"+qtr("Stop")+"  —  "+nativeKey(p,"key-stop");QMessageBox::information(parent,qtr("Keyboard shortcuts"),text);});
        add(qtr("Open media file"),action,[p](){DialogsProvider::getInstance(p)->simpleOpenDialog();},"Ctrl+O");
        add(qtr("Save current moment"),action,[=](){ConeicPersonalDialog::open(p,parent,0)->saveCurrentMoment();},"Ctrl+Shift+M");add(qtr("Your moments"),action,[=](){ConeicPersonalDialog::open(p,parent,0);});add(qtr("Study notes"),action,[=](){ConeicPersonalDialog::open(p,parent,1);},"Ctrl+Shift+N");
        add(qtr("Smart collections"),action,[=](){ConeicPersonalDialog::open(p,parent,2);});add(qtr("Library backup and media relinking"),action,[=](){ConeicLibraryTools::open(p,parent);});add(qtr("Customise Home"),action,[=](){ConeicHomeOptions::open(p,parent);});add(qtr("Settings"),action,[p](){DialogsProvider::getInstance(p)->prefsDialog();},"Ctrl+P");
        QSet<QString> seen;for(QAction *native:parent->findChildren<QAction*>()){QString name=native->text();name.remove('&');if(name.isEmpty()||native->menu()||seen.contains(name))continue;seen.insert(name);QPointer<QAction> guard(native);add(name,qtr("VLC action"),[guard](){if(guard&&guard->isEnabled())guard->trigger();},native->shortcut().toString(QKeySequence::NativeText));}
        auto store=ConeicPersonalStore::instance(p);const auto media=store->media();for(auto it=media.begin();it!=media.end();++it){const auto uri=it.key();add(ConeicPersonalStore::title(uri),qtr("Media"),[=](){const auto url=QUrl::fromEncoded(uri.toUtf8());if(url.isLocalFile()&&!QFileInfo::exists(url.toLocalFile()))ConeicLibraryTools::locate(p,parent,uri);else Open::openMRL(p,uri);});}
        for(auto value:store->moments()){const auto m=value.toObject();add(m.value("title").toString()+" · "+ConeicPersonalStore::title(m.value("uri").toString()),qtr("Saved moment"),[=](){ConeicPersonalDialog::open(p,parent)->playSaved(m);});}
        size_t count=0;module_t **modules=module_list_get(&count);QSet<QString> options;
        for(size_t i=0;i<count;i++){unsigned n=0;auto config=module_config_get(modules[i],&n);if(!config)continue;for(unsigned j=0;j<n;j++){const auto &c=config[j];if(c.i_type<CONFIG_ITEM_FLOAT||c.b_internal||c.b_removed||!c.psz_name||!c.psz_text)continue;const QString name=qfu(c.psz_name);if(options.contains(name))continue;options.insert(name);add(qtr(c.psz_text)+"  ["+name+"]",qtr("Setting"),[=](){DialogsProvider::getInstance(p)->prefsDialog();QTimer::singleShot(120,qApp,[name](){for(QWidget *w:qApp->topLevelWidgets())if(w->inherits("PrefsDialog")){QMetaObject::invokeMethod(w,"setSimple",Qt::DirectConnection);if(auto settings=w->findChild<QWidget*>("ConeicGlobalSettings")){if(auto search=settings->findChild<QLineEdit*>()){search->setText(name);search->setFocus();}break;}}});});}module_config_free(config);}module_list_free(modules);
        query->installEventFilter(this);connect(query,&QLineEdit::textChanged,this,[this](){filter();});connect(query,&QLineEdit::returnPressed,this,[this](){run();});connect(results,&QListWidget::itemActivated,this,[this](QListWidgetItem*){run();});filter();query->setFocus();
    }
    static ConeicCommandPalette *open(intf_thread_t *p,QWidget *parent){for(auto w:qApp->topLevelWidgets())if(w->isVisible()&&w->objectName()=="ConeicCommandPalette"){auto existing=static_cast<ConeicCommandPalette*>(w);existing->raise();existing->query->setFocus();existing->query->selectAll();return existing;}auto palette=new ConeicCommandPalette(p,parent);palette->show();palette->raise();return palette;}
};
#endif
