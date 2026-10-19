/* Copyright (C) 2026 Coneic contributors. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CONEIC_WINDOWS_ASSOCIATIONS_HPP
#define CONEIC_WINDOWS_ASSOCIATIONS_HPP
#ifdef _WIN32
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>

static inline void coneicOpenDefaultApps(QWidget *parent)
{
    const QString directory=QCoreApplication::applicationDirPath();
    const QString helper=directory+QStringLiteral("/Coneic-shell/register-file-associations.ps1");
    if(!QFileInfo::exists(helper)) {
        QMessageBox::information(parent,qtr("Default apps"),qtr("This copy is missing its Windows registration helper. Install a complete Coneic package to enable file associations."));
        return;
    }
    QProcess *process=new QProcess(parent);
    QObject::connect(process,static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),parent,
        [parent,process](int code,QProcess::ExitStatus status){
            if(code!=0 || status!=QProcess::NormalExit)
                QMessageBox::warning(parent,qtr("Default apps"),qtr("Windows could not register Coneic. Try Set Coneic as default.cmd in the application folder."));
            process->deleteLater();
        });
    QObject::connect(process,&QProcess::errorOccurred,parent,[parent,process](QProcess::ProcessError error){
        if(error==QProcess::FailedToStart) {
            QMessageBox::warning(parent,qtr("Default apps"),qtr("Windows PowerShell could not start. Open Default apps in Windows Settings to select Coneic."));
            process->deleteLater();
        }
    });
    const QString powershell=QString::fromLocal8Bit(qgetenv("SystemRoot"))+QStringLiteral("/System32/WindowsPowerShell/v1.0/powershell.exe");
    process->start(powershell,QStringList()<<"-NoProfile"<<"-NonInteractive"<<"-ExecutionPolicy"<<"Bypass"<<"-File"<<helper<<"-AppDirectory"<<directory<<"-OpenSettings");
}
#endif
#endif
