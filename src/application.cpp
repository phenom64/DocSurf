/**************************************************************************
*   Copyright (C) 2013 by Robert Metsaranta                               *
*   therealestrob@gmail.com                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/


#include "application.h"
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

static QString name = "kdfm_browser";

Application::Application(int &argc, char *argv[]) : QApplication(argc, argv)
{
    m_server = new QLocalServer(this);
    m_socket = new QLocalSocket(this);

    const QString &key = name;
    m_socket->connectToServer(key);
    if (m_socket->error() == QLocalSocket::ConnectionRefusedError) //we are assuming a crash happened...
        m_server->removeServer(key);
    m_isRunning = m_socket->state() == QLocalSocket::ConnectedState && m_socket->error() != QLocalSocket::ConnectionRefusedError;

    if (!m_isRunning)
    {
        if (!m_server->listen(name))
            qDebug() << "Wasnt able to start the localServer... IPC will not work right";
        setOrganizationName("dfm");
    }
}



