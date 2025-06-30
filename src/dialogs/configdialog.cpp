#include "configdialog.h"
#include "ui_configdialog.h"
#include "../application.h"

#include <QToolButton>
#include <KF5/KIOCore/KIO/CopyJob>
#include <KF5/KIOFileWidgets/KAbstractViewAdapter>
#include <KF5/KIOFileWidgets/KFilePreviewGenerator>
#include <KF5/KIOCore/KIO/Job>
#include <KF5/KIOWidgets/KIO/PreviewJob>
#include <KF5/KConfigCore/KSharedConfig>
#include <KF5/KConfigCore/KConfigGroup>
#include <KF5/KIOCore/KFileItem>
#include <KF5/KIOCore/KCoreDirLister>
#include <KF5/KIOCore/KDirNotify>
#include <KF5/KIOCore/KIO/ListJob>
#include <QFileDialog>
#include <QUrl>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QDebug>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCheckBox>

class ConfigDialog::Private
{
public:
    Private(ConfigDialog *dialog)
        : q(dialog)
        , ui(new Ui::ConfigDialog)
    {
        ui->setupUi(q);
    }
    ConfigDialog * const q;
    Ui::ConfigDialog *ui;
    void readSettings()
    {
        //Config pointer
        KSharedConfigPtr config = KSharedConfig::openConfig("kdfm.conf");

        //Startup
        KConfigGroup general = config->group("General");

        const QString &location = general.readEntry("Location", QString());
        if (!location.isEmpty())
            ui->txtStartupPath->setText(location);
         ui->hideTabBar->setChecked(general.readEntry("HideTabBar", false));

        //Views
        KConfigGroup views = config->group("Views");
        ui->spbIconViewLines->setValue(views.readEntry("IconTextLines", 3));
        ui->iconSizeSlider->setValue(views.readEntry("IconSize", 48/16));
        connect(ui->iconSizeSlider, &QSlider::valueChanged, [this]() {ui->iconSizeLabel->setText(QString("%1 px").arg(QString::number(ui->iconSizeSlider->value()*16)));});
        ui->iconSizeLabel->setText(QString("%1 px").arg(QString::number(ui->iconSizeSlider->value()*16)));
        ui->categorized->setChecked(views.readEntry("Categorized", false));

        //Previews
        QStringList activePlugins = views.readEntry("PreviewPlugins", KIO::PreviewJob::defaultPlugins());
        QListWidget *pv = ui->previews;
        pv->addItems(KIO::PreviewJob::availablePlugins());
        for (int i = 0; i < pv->count(); ++i)
        {
            QListWidgetItem *item = pv->item(i);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(activePlugins.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
        }
        QCheckBox *remotePreviews = ui->remotePreviews;
        remotePreviews->setChecked(views.readEntry("RemotePreviews", false));
    }
    void writeSettings()
    {
        //Config pointer
        KSharedConfigPtr config = KSharedConfig::openConfig("kdfm.conf");

        //General
        KConfigGroup general = config->group("General");
        QString startPath = ui->txtStartupPath->text();
        if (startPath.isEmpty())
            startPath = QDir::homePath();
        general.writeEntry("Location", startPath);
        general.writeEntry("HideTabBar", ui->hideTabBar->isChecked());

        //Views
        KConfigGroup views = config->group("Views");
        views.writeEntry("IconTextLines", ui->spbIconViewLines->value());
        views.writeEntry("IconSize", ui->iconSizeSlider->value());
        views.writeEntry("Categorized", ui->categorized->isChecked());

        QListWidget *pv = ui->previews;
        QStringList activePlugins;
        for (int i = 0; i < pv->count(); ++i)
            if (pv->item(i)->checkState() == Qt::Checked)
                activePlugins << pv->item(i)->text();
        views.writeEntry("PreviewPlugins", activePlugins);

        views.writeEntry("RemotePreviews", ui->remotePreviews->isChecked());
    }
};

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    d(new Private(this))
{
    d->ui->txtStartupPath->setPlaceholderText(QDir::homePath());
    connect(d->ui->btnStartupPath, &QToolButton::clicked, this, [this]()
    {
        const QString &dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                          QDir::homePath(),
                                                          QFileDialog::ShowDirsOnly
                                                          | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty())
            d->ui->txtStartupPath->setText(dir);
    });
    d->readSettings();
}

ConfigDialog::~ConfigDialog()
{
    delete d->ui;
    delete d;
}

void
ConfigDialog::accept()
{
    d->writeSettings();
    QDialog::accept();
}
