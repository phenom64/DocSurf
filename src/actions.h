#ifndef ACTIONS_H
#define ACTIONS_H

#include <QObject>
#include <QAction>

class KNewFileMenu;
namespace DocSurf
{
class MainWindow;
class ActionContainer
{
public:
    enum Actions { GoBack = 0,
                  GoUp,
                  GoForward,
                  GoHome,
                  Exit,
                  AboutQt,
                  Views_Icon,
                  Views_Detail,
                  Views_Column,
                  Views_Flow,
                  DeleteSelection,
                  ShowHidden,
                  Copy,
                  Cut,
                  Paste,
                  Rename,
                  Refresh,
                  ShowMenuBar,
                  ShowPathBar,
                  AddTab,
                  OpenInTab,
                  Configure,
                  Properties,
                  EditPath,
                  Sort,
                  SortName,
                  SortSize,
                  SortPerm,
                  SortDate,
                  SortDescending,
                  GetFilePath,
                  GetFileName,
                  SplitView,
                  MoveToTrashAction,
                  RestoreFromTrashAction,
                  Stretcher,
                  Filter,
                  ActionCount
                };
    ActionContainer(MainWindow *win = 0);
    ~ActionContainer();
    typedef quint8 Action;
    QAction *action(const Action action) const;
    QActionGroup *sortActs() const;
    QList<QAction *> &trashActions() const;
    QList<QAction *> &spacerActions() const;
    KNewFileMenu *newFileMenu() const;

protected:
    void createActions();

private:
    class Private;
    Private * const d;
    friend class Private;
};
}

#endif // ACTIONS_H
