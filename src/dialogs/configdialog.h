#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConfigDialog(QWidget *parent = 0);
    ~ConfigDialog();
    void accept();

private:
    class Private;
    Private * const d;
    friend class Private;
};

#endif // CONFIGDIALOG_H
