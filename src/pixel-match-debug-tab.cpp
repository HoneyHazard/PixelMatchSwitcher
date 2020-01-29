#include "pixel-match-debug-tab.hpp"
#include "pixel-match-core.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>

PixelMatchDebugTab::PixelMatchDebugTab(
    PixelMatcher *pixelMatcher, QWidget *parent)
: QWidget(parent)
, m_pixelMatcher(pixelMatcher)
{
    QGridLayout *layout = new QGridLayout;

    // status
    QLabel *statusLabel = new QLabel("Status: ", this);
    statusLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addWidget(statusLabel, 0, 0);

    m_statusDisplay = new QLabel("--status--", this);
    m_statusDisplay->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addWidget(m_statusDisplay, 0, 1);

    // find button
    QPushButton *enumButton = new QPushButton("Enum Filters", this);
    connect(enumButton, &QPushButton::released,
            this, &PixelMatchDebugTab::enumReleased);
    layout->addWidget(enumButton, 1, 0);

    // text display
    m_textDisplay = new QTextEdit(this);
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_textDisplay, 2, 0, 2, 2);

    setLayout(layout);
}

void PixelMatchDebugTab::enumReleased()
{
    std::string str = m_pixelMatcher->enumSceneElements();

    m_textDisplay->setText(str.data());
}
