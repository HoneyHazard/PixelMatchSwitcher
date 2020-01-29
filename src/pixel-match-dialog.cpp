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
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));

    // main tab
    QLabel *mainTabTest = new QLabel(obs_module_text("main widget"), this);

    QGridLayout *mainTabLayout = new QGridLayout;
    mainTabLayout->addWidget(mainTabTest, 0, 0);

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
}
