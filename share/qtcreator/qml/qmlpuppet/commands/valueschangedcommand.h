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

#ifndef VALUESCHANGEDCOMMAND_H
#define VALUESCHANGEDCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class ValuesChangedCommand
{
    friend QDataStream &operator<<(QDataStream &out, const ValuesChangedCommand &command);
    friend QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command);
    friend bool operator ==(const ValuesChangedCommand &first, const ValuesChangedCommand &second);

public:
    ValuesChangedCommand();
    explicit ValuesChangedCommand(const QVector<PropertyValueContainer> &valueChangeVector);

    QVector<PropertyValueContainer> valueChanges() const;
    quint32 keyNumber() const;

    static void removeSharedMemorys(const QVector<qint32> &keyNumberVector);

    void sort();

private:
    QVector<PropertyValueContainer> m_valueChangeVector;
    mutable quint32 m_keyNumber;
};

QDataStream &operator<<(QDataStream &out, const ValuesChangedCommand &command);
QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command);

bool operator ==(const ValuesChangedCommand &first, const ValuesChangedCommand &second);
QDebug operator <<(QDebug debug, const ValuesChangedCommand &instance);
} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ValuesChangedCommand)


#endif // VALUESCHANGEDCOMMAND_H
