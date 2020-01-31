#include "pixel-match-dialog.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-debug-tab.hpp"

#include <obs-module.h>

#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>

PixelMatchDialog::PixelMatchDialog(PixelMatcher *pixelMatcher, QWidget *parent)
: QDialog(parent)
, m_pixelMatcher(pixelMatcher)
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));

    // main tab
    QGridLayout *mainTabLayout = new QGridLayout;

    m_testLabel = new QLabel(this);
    mainTabLayout->addWidget(m_testLabel, 0, 0);

    QWidget *mainTab = new QWidget(this);
    mainTab->setLayout(mainTabLayout);

    // debug tab
    PixelMatchDebugTab *debugTab = new PixelMatchDebugTab(pixelMatcher, this);

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(mainTab, obs_module_text("Main"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(tabWidget);
    setLayout(topLevelLayout);

    // connections
    connect(m_pixelMatcher, &PixelMatcher::newFrameImage,
            this, &PixelMatchDialog::onNewFrameImage);
}

void PixelMatchDialog::onNewFrameImage()
{
    auto pixMap = QPixmap::fromImage(m_pixelMatcher->frameImage());
    m_testLabel->setPixmap(pixMap);
}
