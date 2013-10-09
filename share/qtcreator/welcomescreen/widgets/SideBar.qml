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

import QtQuick 2.1
import QtQuick.Layouts 1.0

ColumnLayout {
    spacing: 0

    property alias currentIndex: tabs.currentIndex

    property alias model: tabs.model

    property int __width: tabs.width + 16 * 2
    id: root

    Item {
        z: 1
        width: __width
        height: tabs.height + 51 * 2

        Image {
            fillMode: Image.Tile
            source: "images/background.png"
            anchors.fill: parent
        }

        Tabs {
            anchors.centerIn: parent
            id: tabs

        }

        Rectangle {
            color: "#737373"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            height: 1
        }
        Rectangle {
            color: "#737373"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -1
            height: 1
            opacity: 0.6
        }
        Rectangle {
            color: "#737373"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -2
            height: 1
            opacity: 0.2
        }
    }

    Rectangle {
        color: "#ebebeb"
        width: root.__width
        Layout.minimumHeight: 320
        Layout.fillHeight: true

        Column {
            spacing: 14
            width: parent.width - 16 * 2
            x: 12

            Item {
                height: 24
                width: parent.width
            }

            Text {
                text: qsTr("New to Qt?")
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                text: qsTr("Get Started Now!")
                font.pixelSize: 14
                font.bold: true
            }

            Text {
                text: qsTr("Learn how to develop your own applications and explore Qt Creator.")
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                width: parent.width
            }

            Item {
                height: 8
                width: parent.width
            }

            Button {
                text: qsTr("Get Started")
                onClicked: gettingStarted.openSplitHelp("qthelp://org.qt-project.qtcreator/doc/creator-getting-started.html")
            }

            Item {
                height: 8
                width: parent.width
            }

            Column {
                x: 14
                RowLayout {
                    spacing: 7
                    Rectangle {
                        width: 16
                        height: 16
                        color: "#4e4e4e"
                    }
                    LinkedText {
                        text: qsTr("Online Community")
                        font.pixelSize: 11
                        color: "black"
                        onClicked: gettingStarted.openUrl("http://qt-project.org/forums")
                    }
                }
                RowLayout {
                    spacing: 7
                    Rectangle {
                        width: 16
                        height: 16
                        color: "#4e4e4e"
                    }
                    LinkedText {
                        text: qsTr("Blogs")
                        font.pixelSize: 11
                        color: "black"
                        onClicked: gettingStarted.openUrl("http://planet.qt-project.org")
                    }
                }
                RowLayout {
                    spacing: 7
                    Rectangle {
                        width: 16
                        height: 16
                        color: "#4e4e4e"
                    }
                    LinkedText {
                        text: qsTr("User Guide")
                        font.pixelSize: 11
                        color: "black"
                        onClicked: gettingStarted.openHelp("qthelp://org.qt-project.qtcreator/doc/index.html")
                    }
                }
            }
        }

    }
}