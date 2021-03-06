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

#include "clangprojectsettings.h"

using namespace ClangCodeModel;

ClangProjectSettings::ClangProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
    , m_pchUsage(PchUse_None)
{
    Q_ASSERT(project);

    connect(project, SIGNAL(settingsLoaded()),
            this, SLOT(pullSettings()));
    connect(project, SIGNAL(aboutToSaveSettings()),
            this, SLOT(pushSettings()));
}

ClangProjectSettings::~ClangProjectSettings()
{
}

ProjectExplorer::Project *ClangProjectSettings::project() const
{
    return m_project;
}

ClangProjectSettings::PchUsage ClangProjectSettings::pchUsage() const
{
    return m_pchUsage;
}

void ClangProjectSettings::setPchUsage(ClangProjectSettings::PchUsage pchUsage)
{
    if (pchUsage < PchUse_None || pchUsage > PchUse_Custom)
        return;

    if (m_pchUsage != pchUsage) {
        m_pchUsage = pchUsage;
        emit pchSettingsChanged();
    }
}

QString ClangProjectSettings::customPchFile() const
{
    return m_customPchFile;
}

void ClangProjectSettings::setCustomPchFile(const QString &customPchFile)
{
    if (m_customPchFile != customPchFile) {
        m_customPchFile = customPchFile;
        emit pchSettingsChanged();
    }
}

static QLatin1String PchUsageKey("PchUsage");
static QLatin1String CustomPchFileKey("CustomPchFile");
static QLatin1String SettingsNameKey("ClangProjectSettings");

void ClangProjectSettings::pushSettings()
{
    QVariantMap settings;
    settings[PchUsageKey] = m_pchUsage;
    settings[CustomPchFileKey] = m_customPchFile;

    QVariant s(settings);
    m_project->setNamedSettings(SettingsNameKey, s);
}

void ClangProjectSettings::pullSettings()
{
    QVariant s = m_project->namedSettings(SettingsNameKey);
    QVariantMap settings = s.toMap();

    const PchUsage storedPchUsage = static_cast<PchUsage>(
                settings.value(PchUsageKey, PchUse_Unknown).toInt());
    if (storedPchUsage != PchUse_Unknown)
        setPchUsage(storedPchUsage);
    setCustomPchFile(settings.value(CustomPchFileKey).toString());
}
