#ifndef ABOUTINFO_H
#define ABOUTINFO_H

#include <QDialog>

class QLabel;

class DocSurfAboutInfo : public QDialog
{
    Q_OBJECT

public:
    explicit DocSurfAboutInfo(QWidget *parent = 0);
    ~DocSurfAboutInfo();

private:
    void setupUi();

    QLabel *iconLabel;
    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QLabel *versionLabel;
    QLabel *copyrightLabel;
};

#endif // ABOUTINFO_H
