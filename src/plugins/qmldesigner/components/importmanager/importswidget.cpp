/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "importswidget.h"

#include <QVBoxLayout>

#include "importlabel.h"

namespace QmlDesigner {

ImportsWidget::ImportsWidget(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle(tr("Import Manager"));
}

void ImportsWidget::removeAllImports()
{
    qDeleteAll(m_importLabels);
    m_importLabels.clear();
    updateLayout();
}

static bool importLess(const Import &firstImport, const Import &secondImport)
{
    if (firstImport.url() == "QtQuick")
        return true;

    if (secondImport.url() == "QtQuick")
        return false;

    if (firstImport.isLibraryImport() && secondImport.isFileImport())
        return true;

    if (firstImport.isFileImport() && secondImport.isLibraryImport())
        return false;

    if (firstImport.isFileImport() && secondImport.isFileImport())
        return QString::localeAwareCompare(firstImport.file(), secondImport.file()) < 0;

    if (firstImport.isLibraryImport() && secondImport.isLibraryImport())
        return QString::localeAwareCompare(firstImport.url(), secondImport.url()) < 0;

    return false;
}

void ImportsWidget::setImports(const QList<Import> &imports)
{
    qDeleteAll(m_importLabels);
    m_importLabels.clear();

    QList<Import> sortedImports = imports;

    qSort(sortedImports.begin(), sortedImports.end(), importLess);

    foreach (const Import &import, sortedImports) {
        ImportLabel *importLabel = new ImportLabel(this);
        importLabel->setImport(import);
        m_importLabels.append(importLabel);
        connect(importLabel, SIGNAL(removeImport(Import)), this, SIGNAL(removeImport(Import)));
    }

    updateLayout();
}

void ImportsWidget::updateLayout()
{
    delete layout();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);

    foreach (ImportLabel *importLabel, m_importLabels)
        layout->addWidget(importLabel);

    layout->addStretch();
}

} // namespace QmlDesigner