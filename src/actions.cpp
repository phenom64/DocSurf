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

#include <QMenuBar>
#include <QButtonGroup>
#include <KToolBar>
#include <QContextMenuEvent>
#include <QStatusBar>
#include <QToolButton>
#include <QAction>
#include <QWidgetAction>
#include <QApplication>

#include <KStandardAction>
#include <KActionCollection>
#include <KF5/KIOWidgets/KFileItemActions>
#include <KNewFileMenu>

#include "searchbox.h"
#include "actions.h"
#include "searchbox.h"
#include "mainwindow.h"

using namespace KDFM;

class WidgetAction : public QWidgetAction
{
    Q_OBJECT
public:
    explicit WidgetAction(QObject *parent, const int space = 0, const QString &objectName = QString(), const QString &text = QString())
        : QWidgetAction(parent)
        , m_space(space)
        , m_spacer(0)
        , m_widget(0)
    {
        if (!objectName.isEmpty())
            setObjectName(objectName);
        if (!text.isEmpty())
            setText(text);
    }
    QWidget *createWidget(QWidget *parent)
    {
        if (objectName() == "actionStretcher")
        {
            QWidget *stretch = new QWidget(parent);
            stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            return stretch;
        }
        if (objectName() == "actionFilter")
        {
            if (!m_widget)
            {
                m_widget = new SearchBox(parent);
                setDefaultWidget(m_widget);
                m_widget->setParent(parent);
            }
            return m_widget;
        }
        if (m_space)
        {
            m_spacer = new QWidget(parent);
            if (QToolBar *toolBar = qobject_cast<QToolBar *>(parent))
            {
                connect(toolBar, &QToolBar::orientationChanged, this, &WidgetAction::slotOrientationChanged);
                slotOrientationChanged(toolBar->orientation());
            }
            return m_spacer;
        }
        return 0;
    }
    QWidget *widget() { return m_widget; }
protected slots:
    void slotOrientationChanged(const Qt::Orientation &o)
    {
        if (!m_spacer)
            return;
        if (o == Qt::Horizontal)
            m_spacer->setFixedSize(m_space, 1);
        else
            m_spacer->setFixedSize(1, m_space);
    }

private:
    int m_space;
    QWidget *m_spacer, *m_widget;
};

class ActionContainer::Private
{
public:
    Private(ActionContainer *actions) : q(actions) {}
    ActionContainer *const q;
    QAction *actions[ActionContainer::ActionCount];
    QActionGroup *sortActs;
    QList<QAction *> trashActs, spaceActs;
    QMenu *sortMenu;
    KNewFileMenu *newFileMenu;
    MainWindow *win;
    void addToCollection(const ActionContainer::Action a, const QKeySequence shortCut = QKeySequence())
    {
        win->actionCollection()->addAction(actions[a]->objectName(), actions[a]);
        if (shortCut.count())
            win->actionCollection()->setDefaultShortcut(actions[a], shortCut);
    }
};

ActionContainer::ActionContainer(MainWindow *win)
    : d(new Private(this))
{
    d->sortMenu = new QMenu();
    d->win = win;
    createActions();
}

ActionContainer::~ActionContainer()
{
    delete d;
}

QAction *ActionContainer::action(const Action action) const { return d->actions[action]; }
QActionGroup *ActionContainer::sortActs() const { return d->sortActs; }
QList<QAction *> &ActionContainer::trashActions() const { return d->trashActs; }
QList<QAction *> &ActionContainer::spacerActions() const { return d->spaceActs; }
KNewFileMenu *ActionContainer::newFileMenu() const { return d->newFileMenu; }

void
ActionContainer::createActions()
{
    d->actions[GoHome] = new QAction(QObject::tr("&Go Home"));
    d->actions[GoHome]->setObjectName("actionGoHome");
    d->addToCollection(GoHome);

    d->actions[GoBack] = new QAction(QObject::tr("&Go Back"));
    d->actions[GoBack]->setObjectName("actionGoBack");
    d->addToCollection(GoBack, QKeySequence::Back);

    d->actions[GoUp] = new QAction(QIcon::fromTheme("go-up"), QObject::tr("&Go Up"));
    d->actions[GoUp]->setObjectName("actionGoUp");
    d->addToCollection(GoUp, QKeySequence("Alt+Up"));

    d->actions[GoForward] = new QAction(QObject::tr("&Go Forward"));
    d->actions[GoForward]->setObjectName("actionGoForward");
    d->addToCollection(GoForward, QKeySequence::Forward);

    d->actions[Exit] = new QAction(QObject::tr("E&xit"));
    d->actions[Exit]->setObjectName("actionExit");
    d->addToCollection(Exit, QKeySequence::Quit);

    d->actions[AboutQt] = new QAction(QObject::tr("About &Qt"));
    d->actions[AboutQt]->setObjectName("actionAboutQt");
    d->addToCollection(AboutQt);

    QActionGroup *viewActs = new QActionGroup(d->win->actionCollection());
    viewActs->setExclusive(true);
    d->actions[Views_Icon] = viewActs->addAction(new QAction(QObject::tr("&Icons View")));
    d->actions[Views_Icon]->setObjectName("actionViews_Icon");
    d->actions[Views_Icon]->setCheckable(true);
    d->actions[Views_Icon]->setChecked(true);
    d->addToCollection(Views_Icon, QKeySequence("Ctrl+1"));

    d->actions[Views_Detail] = viewActs->addAction(new QAction(QObject::tr("&Details View")));
    d->actions[Views_Detail]->setObjectName("actionViews_Detail");
    d->actions[Views_Detail]->setCheckable(true);
    d->addToCollection(Views_Detail, QKeySequence("Ctrl+2"));

    d->actions[Views_Column] = viewActs->addAction(new QAction(QObject::tr("&Columns View")));
    d->actions[Views_Column]->setObjectName("actionViews_Column");
    d->actions[Views_Column]->setCheckable(true);
    d->addToCollection(Views_Column, QKeySequence("Ctrl+3"));

    d->actions[Views_Flow] = viewActs->addAction(new QAction(QObject::tr("&Flow")));
    d->actions[Views_Flow]->setObjectName("actionViews_Flow");
    d->actions[Views_Flow]->setCheckable(true);
    d->addToCollection(Views_Flow, QKeySequence("Ctrl+4"));

    d->actions[ShowHidden] = new QAction(QObject::tr("&Show Hidden"));
    d->actions[ShowHidden]->setObjectName("actionShowHidden");
    d->actions[ShowHidden]->setCheckable(true);
    d->addToCollection(ShowHidden, QKeySequence("F8"));

    d->actions[Copy] = new QAction(QIcon::fromTheme("edit-copy"), QObject::tr("&Copy"));
    d->actions[Copy]->setObjectName("actionCopy");
    d->addToCollection(Copy, QKeySequence::Copy);

    d->actions[Cut] = new QAction(QIcon::fromTheme("edit-cut"), QObject::tr("&Cut"));
    d->actions[Cut]->setObjectName("actionCut");
    d->addToCollection(Cut, QKeySequence("Ctrl+X"));

    d->actions[Paste] = new QAction(QIcon::fromTheme("edit-paste"), QObject::tr("&Paste"));
    d->actions[Paste]->setObjectName("actionPaste");
    d->addToCollection(Paste, QKeySequence::Paste);

    d->actions[Rename] = new QAction(QIcon::fromTheme("edit-rename"), QObject::tr("&Rename"));
    d->actions[Rename]->setObjectName("actionRename");
    d->addToCollection(Rename, QKeySequence("F2"));

    d->actions[Refresh] = new QAction(QIcon::fromTheme("view-refresh"), QObject::tr("&Refresh"));
    d->actions[Refresh]->setObjectName("actionRefresh");
    d->addToCollection(Refresh, QKeySequence("F5"));

    d->actions[ShowMenuBar] = new QAction(QObject::tr("Show MenuBar"));
    d->actions[ShowMenuBar]->setObjectName("actionShowMenuBar");
    d->actions[ShowMenuBar]->setCheckable(true);
    d->addToCollection(ShowMenuBar, QKeySequence("Ctrl+M"));

    d->actions[ShowPathBar] = new QAction(QObject::tr("Show Path"));
    d->actions[ShowPathBar]->setObjectName("actionShowPathBar");
    d->actions[ShowPathBar]->setCheckable(true);
    d->addToCollection(ShowPathBar, QKeySequence("Ctrl+P"));

    d->actions[AddTab] = new QAction(QObject::tr("New Tab"));
    d->actions[AddTab]->setObjectName("actionAddTab");
    d->addToCollection(AddTab, QKeySequence("Ctrl+T"));

    d->actions[OpenInTab] = new QAction(QObject::tr("Open In New Tab"));
    d->actions[OpenInTab]->setObjectName("actionOpenInTab");
    d->addToCollection(OpenInTab);

    d->actions[Configure] = new QAction(QObject::tr("Preferences..."));
    d->actions[Configure]->setObjectName("actionConfigure");
    d->addToCollection(Configure);

    d->actions[Properties] = new QAction(QObject::tr("Get Info"));
    d->actions[Properties]->setObjectName("actionProperties");
    d->addToCollection(Properties);

    d->actions[EditPath] = new QAction(QObject::tr("Edit Path"));
    d->actions[EditPath]->setObjectName("actionEditPath");
    d->actions[EditPath]->setCheckable(true);
    d->addToCollection(EditPath, QKeySequence("F6"));

    d->actions[SortDescending] = new QAction(QObject::tr("Descending"));
    d->actions[SortDescending]->setObjectName("actionSortDescending");
    d->actions[SortDescending]->setCheckable(true);
    d->addToCollection(SortDescending);

    d->sortActs = new QActionGroup(d->win);
    d->sortActs->setExclusive(true);
    d->actions[SortName] = d->sortActs->addAction(QObject::tr("Name"));
    d->actions[SortSize] = d->sortActs->addAction(QObject::tr("Size"));
    d->actions[SortDate] = d->sortActs->addAction(QObject::tr("Date"));
    d->actions[SortPerm] = d->sortActs->addAction(QObject::tr("Permissions"));
    foreach (QAction *a, d->sortActs->actions())
        a->setCheckable(true);

    d->sortMenu->setSeparatorsCollapsible(false);
    d->sortMenu->addSeparator()->setText(QObject::tr("Sort By:"));
    d->sortMenu->addActions(d->sortActs->actions());
    d->sortMenu->addSeparator();
    d->sortMenu->addAction(d->actions[SortDescending]);
    d->actions[Sort] = new QAction(QObject::tr("Sort..."));
    d->actions[Sort]->setObjectName("actionSort");
    d->actions[Sort]->setMenu(d->sortMenu);
    d->addToCollection(Sort);

    d->actions[GetFileName] = new QAction(QObject::tr("Copy File name(s) to Clipboard"));
    d->actions[GetFileName]->setObjectName("actionGetFileName");
    d->addToCollection(GetFileName, QKeySequence("Ctrl+Shift+N"));

    d->actions[GetFilePath] = new QAction(QObject::tr("Copy File path(s) to Clipboard"));
    d->actions[GetFilePath]->setObjectName("actionGetFilePath");
    d->addToCollection(GetFilePath, QKeySequence("Ctrl+Shift+P"));

    d->actions[SplitView] = new QAction(QIcon::fromTheme("view-split-left-right"), QObject::tr("Split View"));
    d->actions[SplitView]->setObjectName("actionSplitView");
    d->actions[SplitView]->setCheckable(true);
    d->addToCollection(SplitView);

    d->actions[MoveToTrashAction] = new QAction(QObject::tr("Move Selected File(s) To trash"));
    d->actions[MoveToTrashAction]->setObjectName("actionMoveToTrash");
    d->addToCollection(MoveToTrashAction, QKeySequence("Del"));

    d->actions[RestoreFromTrashAction] = new QAction(QObject::tr("Restore"));
    d->actions[RestoreFromTrashAction]->setObjectName("actionRestoreFromTrash");
    d->trashActs << d->actions[RestoreFromTrashAction];
    d->addToCollection(RestoreFromTrashAction);

    d->actions[DeleteSelection] = new QAction(QIcon::fromTheme("edit-delete"), QObject::tr("&Delete"));
    d->actions[DeleteSelection]->setObjectName("actionDeleteSelection");
    d->trashActs << d->actions[DeleteSelection];
    d->addToCollection(DeleteSelection, QKeySequence("Shift+Del"));

    d->actions[Stretcher] = new WidgetAction(d->win);
    d->actions[Stretcher]->setText("Stretcher...");
    d->actions[Stretcher]->setToolTip("Stretcher so the user can put widgets on the right side of the toolbar");
    d->actions[Stretcher]->setObjectName("actionStretcher");
    d->addToCollection(Stretcher);

    d->actions[Filter] = new WidgetAction(d->win);
    d->actions[Filter]->setText("FilterBar");
    d->actions[Filter]->setObjectName("actionFilter");
    d->addToCollection(Filter);

    const QString &spacerObjectName("actionSpacer-%1-%2");
    const QString &spacerText("Spacer-%1px-%2");
    const int count = 5;
    for (int i = 1; i < count; ++i)
    for (int y = 0; y < count; ++y)
    {
        QAction *spacer = new WidgetAction(0,
                                           i*10,
                                           spacerObjectName.arg(QString::number(i*10), QString::number(y)),
                                           spacerText.arg(QString::number(i*10),QString::number(y)));
        d->spaceActs << spacer;
        d->win->actionCollection()->addAction(spacer->objectName(), spacer);
    }
    d->newFileMenu = new KNewFileMenu(d->win->actionCollection(), "actionNewFile", d->win);
}

#include "actions.moc"
