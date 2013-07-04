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

#ifndef INFORMATIONCONTAINER_H
#define INFORMATIONCONTAINER_H

#include <QDataStream>
#include <QMetaType>
#include <QVariant>
#include <QString>

#include "commondefines.h"

namespace QmlDesigner {

class InformationContainer
{
    friend QDataStream &operator>>(QDataStream &in, InformationContainer &container);
    friend QDataStream &operator<<(QDataStream &out, const InformationContainer &container);
    friend bool operator ==(const InformationContainer &first, const InformationContainer &second);
    friend bool operator <(const InformationContainer &first, const InformationContainer &second);

public:
    InformationContainer();
    InformationContainer(qint32 instanceId,
                         InformationName name,
                         const QVariant &information,
                         const QVariant &secondInformation = QVariant(),
                         const QVariant &thirdInformation = QVariant());

    qint32 instanceId() const;
    InformationName name() const;
    QVariant information() const;
    QVariant secondInformation() const;
    QVariant thirdInformation() const;

private:
    qint32 m_instanceId;
    qint32 m_name;
    QVariant m_information;
    QVariant m_secondInformation;
    QVariant m_thirdInformation;
};

QDataStream &operator<<(QDataStream &out, const InformationContainer &container);
QDataStream &operator>>(QDataStream &in, InformationContainer &container);

bool operator ==(const InformationContainer &first, const InformationContainer &second);
bool operator <(const InformationContainer &first, const InformationContainer &second);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InformationContainer)

#endif // INFORMATIONCONTAINER_H
