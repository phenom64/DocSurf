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


#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

#define dApp static_cast<Application*>(QApplication::instance())
//#define DPY QX11Info::display()

class QLocalSocket;
class QLocalServer;
class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char *argv[]);
    inline bool isRunning() { return m_isRunning; }
    void emitSettingsChanged() { emit settingsChanged(); }

signals:
    void settingsChanged();

private:
    bool m_isRunning;
    QLocalSocket *m_socket;
    QLocalServer *m_server;
};

#endif // APPLICATION_H
