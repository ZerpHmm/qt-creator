/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLANGPROJECTSETTINGS_H
#define CLANGPROJECTSETTINGS_H

#include "clang_global.h"

#include <projectexplorer/project.h>

#include <QObject>
#include <QString>

namespace ClangCodeModel {

class CLANG_EXPORT ClangProjectSettings: public QObject
{
    Q_OBJECT

public:
    enum PchUsage {
        PchUse_Unknown = 0,
        PchUse_None = 1,
        PchUse_BuildSystem_Exact = 2,
        PchUse_BuildSystem_Fuzzy = 3,
        PchUse_Custom = 4
    };

public:
    ClangProjectSettings(ProjectExplorer::Project *project);
    virtual ~ClangProjectSettings();

    ProjectExplorer::Project *project() const;

    PchUsage pchUsage() const;
    void setPchUsage(PchUsage pchUsage);

    QString customPchFile() const;
    void setCustomPchFile(const QString &customPchFile);

signals:
    void pchSettingsChanged();

public slots:
    void pullSettings();
    void pushSettings();

private:
    ProjectExplorer::Project *m_project;
    PchUsage m_pchUsage;
    QString m_customPchFile;
};

} // ClangCodeModel namespace

#endif // CLANGPROJECTSETTINGS_H
