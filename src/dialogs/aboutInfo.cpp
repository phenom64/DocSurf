#include "aboutInfo.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>

DocSurfAboutInfo::DocSurfAboutInfo(QWidget *parent) : QDialog(parent)
{
    setupUi();
}

DocSurfAboutInfo::~DocSurfAboutInfo()
{
}

void DocSurfAboutInfo::setupUi()
{
    // Sets the window title and flags to get a standard NSE.dialogWindow
    // with only a close button, respecting SynOS system theme.
    setWindowTitle("About Document Surfer");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() | Qt::WindowCloseButtonHint);
    setFixedSize(400, 250); // A fixed size is good for an "About" window

    // Use a standard vertical layout. It will inherit the background
    // from your SynOS theme automatically.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(5);
    layout->setAlignment(Qt::AlignCenter);

    // Icon
    // READ: Must add icon to a Qt resource file (.qrc)
    // for this path to work.
    iconLabel = new QLabel(this);
    QPixmap iconPixmap(":/synos/icons/DocSurf.png");
    iconLabel->setPixmap(iconPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);
    layout->addSpacing(10);

    // Labels with font styling. This respects the system font family.
    titleLabel = new QLabel("Document Surfer", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    subtitleLabel = new QLabel("The Syndromatic Desktop Experience\n", this);
    subtitleLabel->setStyleSheet("font-size: 14px;");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitleLabel);

    versionLabel = new QLabel("DocSurf version 1.0 beta", this);
    versionLabel->setStyleSheet("font-size: 12px; color: #555;");
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    // This spacer pushes the NSE.copyrightLabel widget to the bottom
    layout->addStretch();

    copyrightLabel = new QLabel("™ and © 2025 Syndromatic Ltd.\nAll rights reserved.", this);
    copyrightLabel->setStyleSheet("font-size: 12px; color: #555;");
    copyrightLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(copyrightLabel);

    setLayout(layout);
}
