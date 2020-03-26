#include "pm-debug-tab.hpp"
#include "pm-core.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

#include <obs.h>

#include <sstream>

PmDebugTab::PmDebugTab(
    PmCore *pixelMatcher, QWidget *parent)
: QWidget(parent)
, m_core(pixelMatcher)
{
    QGridLayout *layout = new QGridLayout;
    int row = 0;

    QSizePolicy fixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QSizePolicy minimumPolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // filters
    QLabel *filtersLabel = new QLabel("Filters: ", this);
    filtersLabel->setSizePolicy(fixedPolicy);
    layout->addWidget(filtersLabel, row, 0);

    m_filtersStatusDisplay = new QLabel("--", this);
    m_filtersStatusDisplay->setSizePolicy(minimumPolicy);
    layout->addWidget(m_filtersStatusDisplay, row++, 1);

    // active filter
    QLabel *activeFilterLabel = new QLabel("Active Filter: ", this);
    activeFilterLabel->setSizePolicy(fixedPolicy);
    layout->addWidget(activeFilterLabel, row, 0);

    m_activeFilterDisplay = new QLabel("--", this);
    m_activeFilterDisplay->setSizePolicy(minimumPolicy);
    layout->addWidget(m_activeFilterDisplay, row++, 1);

    // source/filter resolution
    QLabel *sourceResLabel = new QLabel("Source Resolution: ", this);
    sourceResLabel->setSizePolicy(fixedPolicy);
    layout->addWidget(sourceResLabel, row, 0);

    m_sourceResDisplay = new QLabel("--", this);
    m_sourceResDisplay->setSizePolicy(minimumPolicy);
    layout->addWidget(m_sourceResDisplay, row++, 1);

    // filter data resolution
    QLabel *filterDataResLabel = new QLabel("Data Resolution ", this);
    filterDataResLabel->setSizePolicy(fixedPolicy);
    layout->addWidget(filterDataResLabel, row, 0);

    m_filterDataResDisplay = new QLabel("--");
    m_filterDataResDisplay->setSizePolicy(minimumPolicy);
    layout->addWidget(m_filterDataResDisplay, row++, 1);

    // number matched
    QLabel *numMatchedLabel = new QLabel("Number Matched ", this);
    numMatchedLabel->setSizePolicy(fixedPolicy);
    layout->addWidget(numMatchedLabel, row, 0);

    m_matchCountDisplay = new QLabel("--", this);
    m_matchCountDisplay->setSizePolicy(minimumPolicy);
    layout->addWidget(m_matchCountDisplay, row++, 1);

    // find button
    QPushButton *scenesInfoButton = new QPushButton("Scenes Info", this);
    connect(scenesInfoButton, &QPushButton::released,
            this, &PmDebugTab::scenesInfoReleased);
    layout->addWidget(scenesInfoButton, row++, 0);

    // text display
    m_textDisplay = new QTextEdit(this);
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_textDisplay, row, 0, 2, 2);
    row += 2;

    setLayout(layout);

    // periodic update timer
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &PmDebugTab::periodicUpdate);
    timer->start(100);
}

void PmDebugTab::scenesInfoReleased()
{
    std::string str = m_core->scenesInfo();

    m_textDisplay->setText(str.data());
}

void PmDebugTab::periodicUpdate()
{
    if (!isVisible()) return;

    std::ostringstream oss;
    auto filters = m_core->filters();
    if (filters.empty()) {
        oss << "no filters found.";
    } else {
        oss << filters.size() << " filter(s) available.";
    }
    m_filtersStatusDisplay->setText(oss.str().data());

    oss.str("");
    auto fi = m_core->activeFilterRef();
    if (fi.isActive()) {
        oss << obs_source_get_name(fi.sceneSrc()) << " -> "
            << obs_source_get_name(fi.itemSrc()) << " -> "
            << obs_source_get_name(fi.filter());
    } else {
        oss << "no filter is active.";
    }
    m_activeFilterDisplay->setText(oss.str().data());

    auto activeFilter = m_core->activeFilterRef();
    if (activeFilter.isValid()) {
        oss.str("");
        oss << activeFilter.filterSrcWidth() << " x "
            << activeFilter.filterSrcHeight();
        m_sourceResDisplay->setText(oss.str().data());

        oss.str("");
        oss << activeFilter.filterDataWidth() << " x "
            << activeFilter.filterDataHeight();
        m_filterDataResDisplay->setText(oss.str().data());

        oss.str("");
        oss << activeFilter.numMatched();
        m_matchCountDisplay->setText(oss.str().data());
    } else {
        m_sourceResDisplay->setText("--");
        m_filterDataResDisplay->setText("--");
        m_matchCountDisplay->setText("--");
    }
}
