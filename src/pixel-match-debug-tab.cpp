#include "pixel-match-debug-tab.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

PixelMatchDebugTab::PixelMatchDebugTab(
    PixelMatcher *pixelMatcher, QWidget *parent)
: QWidget(parent)
, m_pixelMatcher(pixelMatcher)
{
    QGridLayout *layout = new QGridLayout;

    // status
    QLabel *statusLabel = new QLabel("Status: ", this);
    layout->addWidget(statusLabel, 0, 0);

    m_statusDisplay = new QLabel("--status--", this);
    layout->addWidget(m_statusDisplay, 0, 1);

    // find button
    QPushButton *findButton = new QPushButton("Find Filter", this);
    connect(findButton, &QPushButton::released,
            this, &PixelMatchDebugTab::findReleased);
    layout->addWidget(findButton, 1, 0);

    setLayout(layout);
}

void PixelMatchDebugTab::findReleased()
{

}
