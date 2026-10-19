/* Coneic global settings: Juxtopposed's category layout, native VLC bindings.
 * Copyright (C) 2026 Coneic contributors. SPDX-License-Identifier: GPL-2.0-or-later
 * Design attribution: pixmaps/coneic-reference/ATTRIBUTION.md (CC BY 4.0).
 */
#ifndef CONEIC_SETTINGS_HPP
#define CONEIC_SETTINGS_HPP
#include "components/preferences_widgets.hpp"
#include "components/simple_preferences.hpp"
#include "coneic_design.hpp"
#include "coneic_windows_associations.hpp"
#include <vlc_modules.h>
#include <vlc_plugin.h>
#include <vlc_config_cat.h>
#include <QListWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QAbstractSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QToolTip>
#include <QSet>
#include <QMap>
#include <QTreeWidget>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

class ConeicGlobalSettings : public QWidget
{
    struct Option { module_config_t *config; QString module, section; int category; bool basic; };
    struct Row { QWidget *widget; ConfigControl *control; bool dirty; };
    intf_thread_t *intf;
    QList<module_config_t*> configurations;
    QList<Option> options;
    QMap<QString, Row> rows;
    QListWidget *navigation;
    QLineEdit *search;
    QLabel *heading;
    QWidget *body;
    QVBoxLayout *contents;
    QScrollArea *scroll;
    QSet<int> expanded;
    int resultLimit = 150;
    int pendingTheme=-1;
    bool themePreview=false,committed=false;
    QPalette originalPalette;
    QString originalStylesheet;
    QWidget *languageRow = nullptr;
    QWidget *materialRow = nullptr;
    int originalMaterial=0,pendingMaterial=-1;
#ifdef _WIN32
    SPrefsPanel *platformPreferences = nullptr;
#endif
    const QStringList titles = {qtr("General"),qtr("Interface"),qtr("Privacy"),qtr("Controls"),
        qtr("Playback"),qtr("Video"),qtr("Audio"),qtr("Subtitles"),qtr("Library"),
        qtr("Network"),qtr("Performance"),qtr("Recording"),qtr("Extensions")};

    static int categoryFor(const QString &name, int category, const QString &module)
    {
        if(name=="qt-dark-palette" || name=="qt-updates-notif" || name=="qt-error-dialogs")return 0;
        if (name.startsWith("key-") || name.startsWith("global-key-") || name.contains("mouse") || name.contains("hotkey")) return 3;
        if (name.contains("privacy") || name.contains("recent") || name.contains("metadata-network") || name.contains("saveplayed") || name.contains("cookies") || name.contains("password") || name.contains("username")) return 2;
        if (name.startsWith("sub-") || name.startsWith("subs") || name.startsWith("spu") || name.startsWith("freetype") || name.startsWith("osd") || name.startsWith("marq") || name.startsWith("logo-") || name.contains("subtitle")) return 7;
        if (name.startsWith("sout") || name.startsWith("mux") || name.startsWith("record") || name.contains("chromecast") || category == CAT_SOUT) return 11;
        if (name.contains("cache") || name.contains("thread") || name.contains("hardware") || name.contains("hw-") || name.contains("hwaccel") || name.contains("avcodec") || name.startsWith("codec") || name.contains("skip-frame")) return 10;
        if (name.startsWith("http") || name.startsWith("rtsp") || name.startsWith("rtp-") || name.startsWith("udp") || name.startsWith("smb") || name.startsWith("ftp") || name.startsWith("tls") || name.contains("network") || name.contains("proxy") || name.contains("ipv") || name.contains("multicast")) return 9;
        if (name.contains("playlist") || name.startsWith("media-library") || name.startsWith("services-discovery") || category == CAT_PLAYLIST) return 8;
        if (category == CAT_AUDIO || name.startsWith("audio") || name.startsWith("aout") || name.startsWith("equalizer") || name.startsWith("compressor") || name.startsWith("spatializer")) return 6;
        if (category == CAT_VIDEO || name.startsWith("video") || name.startsWith("vout") || name.startsWith("fullscreen") || name.startsWith("snapshot")) return 5;
        if (name.contains("skin") || name.startsWith("qt-") || name.startsWith("intf-") || category == CAT_INTERFACE) return 1;
        if (name.contains("lua") || name.contains("plugin") || module.contains("extension", Qt::CaseInsensitive)) return 12;
        if (category == CAT_INPUT || name.contains("rate") || name.contains("repeat") || name.contains("random") || name.contains("play-and")) return 4;
        return 0;
    }

    QWidget *rowFor(const Option &option)
    {
        const QString name = qfu(option.config->psz_name);
        if (rows.contains(name)) return rows[name].widget;
        QWidget *row = new QWidget(body);
        row->setObjectName("ConeicSettingRow");
        row->setProperty("coneicOption", name);
        row->setAccessibleName(qtr(option.config->psz_text));
        QHBoxLayout *horizontal = new QHBoxLayout(row);
        horizontal->setContentsMargins(0,5,0,5);
        QWidget *fields = new QWidget(row);
        QGridLayout *grid = new QGridLayout(fields);
        grid->setContentsMargins(0,0,0,0);
        grid->setColumnStretch(0,1);
        ConfigControl *control = nullptr;
        if(name=="qt-dark-palette") {
            QLabel *label=new QLabel(qtr("Theme"),fields);grid->addWidget(label,0,0);
            QCheckBox *value=new QCheckBox(fields);value->hide();
            control=new BoolConfigControl(VLC_OBJECT(intf),option.config,label,value);
            QWidget *segments=new QWidget(fields);QHBoxLayout *buttons=new QHBoxLayout(segments);buttons->setContentsMargins(0,0,0,0);buttons->setSpacing(2);
            const QStringList names={qtr("Dark"),qtr("Light"),qtr("System")};
            const int current=intf->p_sys->mainSettings->value("Coneic/theme-mode",value->isChecked()?0:1).toInt();
            for(int mode=0;mode<3;mode++) {
                QToolButton *button=new QToolButton(segments);button->setText(names[mode]);button->setCheckable(true);button->setAutoExclusive(true);button->setChecked(current==mode);button->setMinimumWidth(64);buttons->addWidget(button);
                connect(button,&QToolButton::clicked,row,[this,value,mode](){
                    bool dark=mode==2?ConeicDesign::systemDark():mode==0;
                    value->setChecked(dark);pendingTheme=mode;themePreview=true;qApp->setProperty("coneicThemePreview",true);ConeicDesign::apply(*qApp,dark,false);
                });
            }
            grid->addWidget(segments,0,1,Qt::AlignRight);
        } else if(name=="sub-autodetect-fuzzy") {
            QLabel *label=new QLabel(qtr("Match subtitle filenames"),fields);
            QSpinBox *value=new QSpinBox(fields);
            control=new IntegerConfigControl(VLC_OBJECT(intf),option.config,label,value);value->hide();
            QComboBox *choices=new QComboBox(fields);
            const QStringList descriptions={qtr("Do not detect subtitles"),qtr("Any subtitle file"),qtr("Filename contains the movie name"),qtr("Movie name with additional characters"),qtr("Exact movie filename")};
            for(int i=0;i<descriptions.size();i++)choices->addItem(descriptions[i],i);
            if(value->value()<0 || value->value()>4)choices->addItem(qtr("Custom")+QString(" (%1)").arg(value->value()),value->value());
            choices->setCurrentIndex(choices->findData(value->value()));
            label->setBuddy(choices);choices->setAccessibleName(label->text());
            connect(choices,QOverload<int>::of(&QComboBox::currentIndexChanged),value,[value,choices](int index){value->setValue(choices->itemData(index).toInt());});
            grid->addWidget(label,0,0);grid->addWidget(choices,0,1);
        } else if (option.config->i_type == CONFIG_ITEM_BOOL) {
            QLabel *label = new QLabel(qtr(option.config->psz_text), fields);
            label->setWordWrap(true);
            QCheckBox *toggle = new QCheckBox(fields);
            toggle->setAccessibleName(label->text());
            label->setBuddy(toggle);
            control = new BoolConfigControl(VLC_OBJECT(intf),option.config,label,toggle);
            grid->addWidget(label,0,0);
            grid->addWidget(toggle,0,1,Qt::AlignRight);
        } else {
            control = ConfigControl::createControl(VLC_OBJECT(intf),option.config,fields,grid,0);
        }
        horizontal->addWidget(fields,1);
        if (!control) { delete row; return nullptr; }
        if (option.config->psz_longtext) {
            QToolButton *info = new QToolButton(row);
            info->setIcon(QIcon(":/coneic/settings-info.svg"));
            info->setAccessibleName(qtr("More information") + ": " + qtr(option.config->psz_text));
            const QString help = qtr(option.config->psz_longtext);
            info->setToolTip(help);
            info->setFixedSize(24,24);
            connect(info,&QToolButton::clicked,row,[info,help](){QToolTip::showText(info->mapToGlobal(QPoint(0,info->height())),help,info);});
            horizontal->addWidget(info,0,Qt::AlignTop);
        }
        rows.insert(name,{row,control,false});
        // Track changes after initialization: Save must never rewrite untouched options.
        auto dirty = [this,name](){rows[name].dirty=true;};
        for (QAbstractButton *w : fields->findChildren<QAbstractButton*>()) connect(w,&QAbstractButton::clicked,row,dirty);
        for (QLineEdit *w : fields->findChildren<QLineEdit*>()) connect(w,&QLineEdit::textEdited,row,dirty);
        for (QComboBox *w : fields->findChildren<QComboBox*>()) connect(w,QOverload<int>::of(&QComboBox::activated),row,dirty);
        for (QSpinBox *w : fields->findChildren<QSpinBox*>()) connect(w,QOverload<int>::of(&QSpinBox::valueChanged),row,dirty);
        for (QDoubleSpinBox *w : fields->findChildren<QDoubleSpinBox*>()) connect(w,QOverload<double>::of(&QDoubleSpinBox::valueChanged),row,dirty);
        for (QAbstractSlider *w : fields->findChildren<QAbstractSlider*>()) connect(w,&QAbstractSlider::valueChanged,row,dirty);
        for (QTreeWidget *w : fields->findChildren<QTreeWidget*>()) connect(w,&QTreeWidget::itemChanged,row,dirty);
        return row;
    }

    void render()
    {
        while (QLayoutItem *item = contents->takeAt(0)) {
            if (QWidget *w=item->widget()) { if(w->property("coneicOption").isValid()) w->hide(); else delete w; }
            delete item;
        }
        const int category = qMax(0,navigation->currentRow());
        const QString query = search->text().trimmed();
        heading->setText(query.isEmpty() ? titles[category] + " " + qtr("Settings") : qtr("Search settings"));
        QString section;
        int count=0, advancedCount=0, matches=0;
#ifdef _WIN32
        if((query.isEmpty() && category==0) || (!query.isEmpty() && QStringLiteral("default apps file associations formats").contains(query,Qt::CaseInsensitive))) {
            QPushButton *defaults=new QPushButton(qtr("Set Coneic as default player"),body);
            defaults->setObjectName("ConeicDefaultApps");
            defaults->setToolTip(qtr("Choose Coneic for audio, video and playlists in Windows Settings."));
            connect(defaults,&QPushButton::clicked,this,[this](){coneicOpenDefaultApps(this);});
            contents->addWidget(defaults);count++;
        }
#endif
        if(languageRow && ((query.isEmpty() && category==0) || (!query.isEmpty() && qtr("Interface language").contains(query,Qt::CaseInsensitive)))) {
            section=query.isEmpty()?qtr("Appearance"):qtr("General");
            QLabel *label=new QLabel(section,body);label->setObjectName("ConeicSectionTitle");contents->addWidget(label);
            contents->addWidget(languageRow);languageRow->show();count++;
        }
        if(materialRow && ((query.isEmpty() && category==0) || (!query.isEmpty() && QStringLiteral("surface material frosted glass transparency solid").contains(query,Qt::CaseInsensitive)))) {
            contents->addWidget(materialRow);materialRow->show();count++;
        }
        QSet<QString> keyControls;
        for (const Option &option : options) {
            if(query.isEmpty() && option.category != category) continue;
            const QString name=qfu(option.config->psz_name);
            const QString description=qtr(option.config->psz_text);
            const QString help=option.config->psz_longtext ? qtr(option.config->psz_longtext) : QString();
            if(!query.isEmpty() && !(name.contains(query,Qt::CaseInsensitive) || description.contains(query,Qt::CaseInsensitive) || help.contains(query,Qt::CaseInsensitive) || option.module.contains(query,Qt::CaseInsensitive))) continue;
            if(option.config->i_type==CONFIG_ITEM_KEY) {
                if(keyControls.contains("keys")) continue;
                keyControls.insert("keys");
            }
            if(query.isEmpty() && !option.basic && !expanded.contains(category)) {advancedCount++;continue;}
            matches++;
            if(!query.isEmpty() && count>=resultLimit) continue;
            const QString group = query.isEmpty() ? option.section : titles[option.category];
            if(group!=section) {
                QLabel *label=new QLabel(group,body); label->setObjectName("ConeicSectionTitle");
                contents->addWidget(label); section=group;
            }
            QWidget *row=rowFor(option);
            if(row) {contents->addWidget(row);row->show();count++;}
        }
        if(advancedCount) {
            QToolButton *advanced=new QToolButton(body);
            advanced->setObjectName("ConeicAdvancedDisclosure");
            advanced->setText(qtr("Advanced Options") + QString("  (%1)  %2").arg(advancedCount).arg(QChar(0x2304)));
            advanced->setToolButtonStyle(Qt::ToolButtonTextOnly);
            advanced->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            connect(advanced,&QToolButton::clicked,this,[this,category](){expanded.insert(category);render();});
            contents->addWidget(advanced);
        }
        if(!query.isEmpty() && matches>count) {
            QPushButton *more=new QPushButton(qtr("Show more") + QString(" (%1)").arg(matches-count),body);
            connect(more,&QPushButton::clicked,this,[this](){resultLimit+=150;render();});
            contents->addWidget(more);
        }
        if(!count && !advancedCount) contents->addWidget(new QLabel(qtr("No matching settings"),body));
        contents->addStretch();
        scroll->verticalScrollBar()->setValue(0);
    }
public:
    ConeicGlobalSettings(intf_thread_t *p, QWidget *parent=nullptr) : QWidget(parent),intf(p)
    {
        setObjectName("ConeicGlobalSettings");
        originalPalette=qApp->palette();originalStylesheet=qApp->styleSheet();
        originalMaterial=qApp->property("coneicMaterialMode").toInt();
        QHBoxLayout *layout=new QHBoxLayout(this); layout->setContentsMargins(14,12,14,12);layout->setSpacing(12);
        QWidget *sidebar=new QWidget(this);sidebar->setObjectName("ConeicSettingsSidebar"); sidebar->setFixedWidth(200);
        QVBoxLayout *side=new QVBoxLayout(sidebar);side->setContentsMargins(10,10,10,10);
        search=new QLineEdit(sidebar);search->setPlaceholderText(qtr("Search"));search->setAccessibleName(qtr("Search all settings"));
        search->setClearButtonEnabled(true);side->addWidget(search);
        navigation=new QListWidget(sidebar);navigation->setObjectName("ConeicSettingsNavigation");navigation->setFrameShape(QFrame::NoFrame);
        const QStringList icons={"general","interface","privacy","controls","playback","video","audio","subtitles","library","network","performance","recording","extensions"};
        for(int i=0;i<titles.size();i++) {auto item=new QListWidgetItem(QIcon(":/coneic/settings-"+icons[i]+".svg"),titles[i],navigation);item->setSizeHint(QSize(180,40));}
        side->addWidget(navigation,1);auto company=new QLabel(QStringLiteral("Made by <a href=\"https://bluecloudai.online\">BLUECLOUD</a>"),sidebar);company->setObjectName("ConeicCompanyLink");company->setOpenExternalLinks(true);company->setWordWrap(true);QFont creditFont=company->font();creditFont.setPointSizeF(8.5);company->setFont(creditFont);company->setContentsMargins(8,12,8,4);side->addWidget(company);layout->addWidget(sidebar);
        QWidget *right=new QWidget(this);QVBoxLayout *content=new QVBoxLayout(right);content->setContentsMargins(24,16,24,16);
        heading=new QLabel(right);heading->setObjectName("ConeicSettingsHeading");content->addWidget(heading);
        scroll=new QScrollArea(right);scroll->setFrameShape(QFrame::NoFrame);scroll->setWidgetResizable(true);
        body=new QWidget;contents=new QVBoxLayout(body);contents->setContentsMargins(0,8,8,8);contents->setSpacing(2);
        scroll->setWidget(body);content->addWidget(scroll,1);layout->addWidget(right,1);
        materialRow=new QWidget(body);materialRow->setProperty("coneicOption","__surface-material");
        auto materialLayout=new QHBoxLayout(materialRow);materialLayout->setContentsMargins(0,8,0,8);
        auto materialLabel=new QLabel(qtr("Surface material"),materialRow);
        auto material=new QComboBox(materialRow);material->setObjectName("ConeicSurfaceMaterial");
        material->addItems({qtr("Follow system"),qtr("Frosted glass"),qtr("Solid")});
        material->setCurrentIndex(qBound(0,originalMaterial,2));material->setAccessibleName(materialLabel->text());materialLabel->setBuddy(material);
        material->setToolTip(qtr("Choose a material for Coneic. Windows high contrast always uses solid system colors."));
        materialLayout->addWidget(materialLabel,1);materialLayout->addWidget(material);
        connect(material,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int mode){
            pendingMaterial=mode;qApp->setProperty("coneicMaterialMode",mode);
            ConeicDesign::apply(*qApp,qApp->property("coneicDark").toBool(),false);
        });
#ifdef _WIN32
        // Keep VLC's native Windows language selection and persistence, separately
        // from engine options. Its existing bindings remain attached to the combo.
        platformPreferences=new SPrefsPanel(intf,this,SPrefsInterface);
        platformPreferences->hide();
        QComboBox *language=platformPreferences->findChild<QComboBox*>(QStringLiteral("langCombo"));
        if(language) {
            languageRow=new QWidget(body);languageRow->setProperty("coneicOption","__interface-language");
            QHBoxLayout *row=new QHBoxLayout(languageRow);row->setContentsMargins(0,8,0,8);
            QLabel *label=new QLabel(qtr("Interface language"),languageRow);label->setBuddy(language);
            row->addWidget(label,1);row->addWidget(language);
            language->setAccessibleName(label->text());language->setToolTip(qtr("Restart Coneic to apply the interface language."));
        }
#endif
        const QSet<QString> basics = QSet<QString>::fromList(QString(
            "language qt-dark-palette qt-privacy-ask qt-updates-notif qt-error-dialogs stats verbose file-logging logmode log-verbose logfile "
            "embedded-video qt-video-autoresize qt-name-in-title qt-start-minimized qt-fs-controller qt-system-tray qt-notification qt-minimal-view video-on-top fullscreen "
            "qt-recentplay qt-continue metadata-network-access input-fast-seek play-and-exit play-and-stop start-paused repeat loop random rate "
            "video vout video-deco video-title-show video-title-position snapshot-path snapshot-format deinterlace deinterlace-mode crop aspect-ratio "
            "audio aout audio-language audio-desync volume audio-time-stretch audio-filter equalizer-preset equalizer-preamp "
            "spu sub-autodetect-file sub-autodetect-fuzzy sub-language subsdec-encoding freetype-font freetype-rel-fontsize freetype-fontsize freetype-color freetype-outline-thickness sub-margin osd "
            "media-library playlist-autostart playlist-tree recursive http-reconnect http-proxy http-user-agent network-caching ipv4-timeout "
            "file-caching disc-caching live-caching avcodec-hw avcodec-threads codec sout-record-path sout-keep sout-transcode-vcodec sout-transcode-acodec lua-intf"
        ).split(' ',QString::SkipEmptyParts));
        size_t moduleCount=0;module_t **modules=module_list_get(&moduleCount);
        QSet<QString> names; bool hasKeys=false;
        for(size_t m=0;m<moduleCount;m++) {
            unsigned size=0;module_config_t *config=module_config_get(modules[m],&size);
            if(!config) continue;configurations.append(config);
            int category=0;QString section=qtr(module_GetLongName(modules[m]));
            for(unsigned n=0;n<size;n++) {
                module_config_t *item=config+n;
                if(item->i_type==CONFIG_CATEGORY) {category=item->value.i;continue;}
                if(item->i_type==CONFIG_SUBCATEGORY) {if(!category && item->value.i>0)category=item->value.i/100;continue;}
                if(item->i_type==CONFIG_SECTION) {if(item->psz_text)section=qtr(item->psz_text);continue;}
                if(item->i_type<CONFIG_ITEM_FLOAT || item->b_internal || item->b_removed || !item->psz_name || !item->psz_text)continue;
                const QString name=qfu(item->psz_name);
                if(names.contains(name))continue;names.insert(name);
                if(item->i_type==CONFIG_ITEM_KEY) {if(hasKeys)continue;hasKeys=true;}
                const bool basic=basics.contains(name)||item->i_type==CONFIG_ITEM_KEY;
                const int mapped=categoryFor(name,category,qfu(module_get_object(modules[m])));
                QString displaySection=section;
                if(mapped==0 && basic)displaySection=name=="qt-dark-palette"?qtr("Appearance"):((name=="qt-updates-notif" || name=="qt-error-dialogs")?qtr("Startup"):qtr("Diagnostics"));
                options.append({item,qtr(module_GetLongName(modules[m])),displaySection,mapped,basic});
            }
        }
        module_list_free(modules);
        std::stable_sort(options.begin(),options.end(),[](const Option&a,const Option&b){
            if(a.category!=b.category)return a.category<b.category;
            if(a.basic!=b.basic)return a.basic;
            if(a.category==0 && a.basic) {
                const QStringList order={qtr("Appearance"),qtr("Startup"),qtr("Diagnostics")};
                const int left=order.indexOf(a.section),right=order.indexOf(b.section);
                if(left!=right)return left<right;
            }
            return a.section.localeAwareCompare(b.section)<0;
        });
        setProperty("coneicOptionCount",options.size());
        const QString evidenceDirectory=QString::fromLocal8Bit(qgetenv("CONEIC_TEST_OUTPUT"));
        if(!evidenceDirectory.isEmpty()) {
            QJsonArray inventory;
            for(const Option &option:options)inventory.append(QJsonObject{
                {QStringLiteral("name"),qfu(option.config->psz_name)},
                {QStringLiteral("label"),qtr(option.config->psz_text)},
                {QStringLiteral("category"),titles[option.category]},
                {QStringLiteral("section"),option.section},
                {QStringLiteral("module"),option.module},
                {QStringLiteral("basic"),option.basic}});
            QFile evidence(evidenceDirectory+QStringLiteral("/settings-option-inventory.json"));
            if(evidence.open(QIODevice::WriteOnly))evidence.write(QJsonDocument(inventory).toJson());
        }
        navigation->setCurrentRow(0);
        connect(navigation,&QListWidget::currentRowChanged,this,[this](){search->clear();render();});
        connect(search,&QLineEdit::textChanged,this,[this](){resultLimit=150;render();});
        render();
    }
    void apply() {
#ifdef _WIN32
        if(platformPreferences)platformPreferences->applyLanguageOnly();
#endif
        applyOptions();
        if(pendingTheme>=0)intf->p_sys->mainSettings->setValue("Coneic/theme-mode",pendingTheme);
        if(pendingMaterial>=0)intf->p_sys->mainSettings->setValue("Coneic/material-mode",pendingMaterial);
        committed=true;
    }
    void applyOptions() {for(auto i=rows.begin();i!=rows.end();++i) if(i.value().dirty)i.value().control->doApply();}
    ~ConeicGlobalSettings() {
        if(pendingMaterial>=0 && !committed)qApp->setProperty("coneicMaterialMode",originalMaterial);
        if(themePreview && !committed){qApp->setPalette(originalPalette);qApp->setStyleSheet(originalStylesheet);qApp->setProperty("coneicDark",originalPalette.color(QPalette::Window).lightness()<128);}
        if(!committed && (themePreview || pendingMaterial>=0))ConeicDesign::apply(*qApp,originalPalette.color(QPalette::Window).lightness()<128,false);
        if(themePreview)qApp->setProperty("coneicThemePreview",false);
        for(const Row &row:rows)delete row.control;for(auto config:configurations)module_config_free(config);
    }
};
#endif
