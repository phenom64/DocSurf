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
#include "mainwindow.h"
#include <KFileItem>

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusInterface>

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    qRegisterMetaType<KFileItem>("KFileItem");
    qRegisterMetaType<KFileItemList>("KFileItemList");
    if (app.isRunning())
    {
        if (app.arguments().count() > 1)
        {
            const QUrl url = QUrl::fromUserInput(app.arguments().at(1));
            if (!url.isValid())
                return 0;
            static const QString destination("org.kde.kdfm");
            static const QString path("/KdfmAdaptor");
            static const QString interface("org.kde.kdfm");
            static const QString method("openUrl");
            QDBusMessage msg = QDBusMessage::createMethodCall(destination, path, interface, method);
            msg << url.toString();
            QDBusConnection::sessionBus().send(msg);
        }
        return 0;
    }
    else
    {
        KDFM::MainWindow *mainWin = new KDFM::MainWindow(app.arguments());
        mainWin->show();
        return app.exec();
    }
}

