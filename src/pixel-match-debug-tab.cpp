#include "pixel-match-debug-tab.hpp"
#include "pixel-match-core.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

#include <sstream>

PixelMatchDebugTab::PixelMatchDebugTab(
    PixelMatcher *pixelMatcher, QWidget *parent)
: QWidget(parent)
, m_pixelMatcher(pixelMatcher)
{
    QGridLayout *layout = new QGridLayout;
    int row = 0;

    // filters
    QLabel *filtersLabel = new QLabel("Filters: ", this);
    filtersLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(filtersLabel, row, 0);

    m_filtersStatusDisplay = new QLabel("--", this);
    m_filtersStatusDisplay->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addWidget(m_filtersStatusDisplay, row++, 1);

    // active filter
    QLabel *activeFilterLabel = new QLabel("Active Filter: ", this);
    activeFilterLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(activeFilterLabel, row, 0);

    m_activeFilterDisplay = new QLabel("--", this);
    m_activeFilterDisplay->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addWidget(m_activeFilterDisplay, row++, 1);

    // find button
    QPushButton *scenesInfoButton = new QPushButton("Scenes Info", this);
    connect(scenesInfoButton, &QPushButton::released,
            this, &PixelMatchDebugTab::scenesInfoReleased);
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
            this, &PixelMatchDebugTab::periodicUpdate);
    timer->start(100);
}

void PixelMatchDebugTab::scenesInfoReleased()
{
    std::string str = m_pixelMatcher->scenesInfo();

    m_textDisplay->setText(str.data());
}

void PixelMatchDebugTab::periodicUpdate()
{
    std::ostringstream oss;
    auto filters = m_pixelMatcher->filters();
    if (filters.empty()) {
        oss << "no filters found.";
    } else {
        oss << filters.size() << " filter(s) available.";
    }
    m_filtersStatusDisplay->setText(oss.str().data());

    oss.str("");
    auto activeFilter = m_pixelMatcher->activeFilter();
    if (activeFilter) {
        OBSSource source = obs_weak_source_get_source(activeFilter);
        //obs_scene_t *scene = obs_scene_from_source(source);
        //obs_source_t *sceneSource = obs_scene_get_source(scene);
        oss << "name = " << obs_source_get_name(source);
            //<< ", scene = " << obs_source_get_name(sceneSource);
    } else {
        oss << "no filter is active.";
    }
    m_activeFilterDisplay->setText(oss.str().data());
}
