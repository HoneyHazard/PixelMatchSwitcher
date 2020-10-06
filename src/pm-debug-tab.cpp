#include "pm-debug-tab.hpp"
#include "pm-core.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

#include <obs.h>

#include <sstream>

PmDebugTab::PmDebugTab(
    PmCore *pixelMatcher, QWidget *parent)
: QDialog(parent, Qt::WindowStaysOnTopHint)
, m_core(pixelMatcher)
{
    setWindowTitle(obs_module_text("Pixel Match Switcher: Debug"));
    QFormLayout *mainLayout = new QFormLayout;
    int row = 0;

    QSizePolicy fixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QSizePolicy minimumPolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // active filter
    m_activeFilterDisplay = new QLabel("--", this);
    m_activeFilterDisplay->setSizePolicy(minimumPolicy);
    mainLayout->addRow("Active Filter: ", m_activeFilterDisplay);

    // filter mode
    m_filterModeDisplay = new QLabel("--", this);
    m_filterModeDisplay->setSizePolicy(minimumPolicy);
    mainLayout->addRow("Filter Mode: ", m_filterModeDisplay);

    // source/filter resolution
    m_sourceResDisplay = new QLabel("--", this);
    m_sourceResDisplay->setSizePolicy(minimumPolicy);
    mainLayout->addRow("Source Resolution: ", m_sourceResDisplay);

    // filter data resolution
    m_filterDataResDisplay = new QLabel("--");
    m_filterDataResDisplay->setSizePolicy(minimumPolicy);
    mainLayout->addRow("Data Resolution: ", m_filterDataResDisplay);

    // number matched
    m_matchCountDisplay = new QLabel("--", this);
    m_matchCountDisplay->setSizePolicy(minimumPolicy);
    mainLayout->addRow("Number Matched: ", m_matchCountDisplay);

    // capture state
    m_captureStateDisplay = new QLabel("--", this);
    m_captureStateDisplay->setSizePolicy(minimumPolicy);
    mainLayout->addRow("Capture State: ", m_captureStateDisplay);

    // preview mode
    m_previewModeDisplay = new QLabel("--", this);
    m_previewModeDisplay->setSizePolicy(minimumPolicy);
    mainLayout->addRow("Preview Mode: ", m_previewModeDisplay);

    // find button
    QPushButton *scenesInfoButton = new QPushButton("Scenes Info", this);
    connect(scenesInfoButton, &QPushButton::released,
            this, &PmDebugTab::scenesInfoReleased);

    // buttons (in case there will be more)
    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(scenesInfoButton);
    mainLayout->addRow(buttonsLayout);

    // text display
    m_textDisplay = new QTextEdit(this);
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addRow(m_textDisplay);

    setLayout(mainLayout);

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
    oss.str("");
    auto fi = m_core->activeFilterRef();
    if (fi.isValid()) {
        oss << obs_source_get_name(fi.filter());
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
        oss << activeFilter.numMatched(0);
        m_matchCountDisplay->setText(oss.str().data());
    } else {
        m_sourceResDisplay->setText("--");
        m_filterDataResDisplay->setText("--");
        m_matchCountDisplay->setText("--");
    }

    {
        QString filterModeStr;
        if (fi.isValid()) {
            fi.lockData();
            auto filterMode = fi.filterData()->filter_mode;
            fi.unlockData();

            switch (filterMode) {
            case PM_MATCH: filterModeStr = "PM_MATCH"; break;
            case PM_MATCH_VISUALIZE: filterModeStr = "PM_MATCH_VISUALIZE"; break;
            case PM_SELECT_REGION_VISUALIZE: filterModeStr = "PM_SELECT_REGION"; break;
            case PM_MASK: filterModeStr = "PM_MASK"; break;
            case PM_MASK_VISUALIZE: filterModeStr = "PM_MASK_VISUALIZE"; break;
            default: 
                filterModeStr = QString("Unknown (%1)").arg((int)filterMode);
            }
            m_filterModeDisplay->setText(filterModeStr);
        }
    }

    {
        auto capState = m_core->captureState();
        QString capStr;
        switch (capState) {
        case PmCaptureState::Inactive: capStr = "Inactive"; break;
        case PmCaptureState::Activated: capStr = "Activated"; break;
        case PmCaptureState::SelectBegin: capStr = "SelectBegin"; break;
        case PmCaptureState::SelectMoved: capStr = "SelectMoved"; break;
        case PmCaptureState::SelectFinished: capStr = "SelectFinished"; break;
        case PmCaptureState::Accepted: capStr = "Accepted"; break;
        case PmCaptureState::Automask: capStr = "Automask"; break;
        default: capStr = QString("Unknown (%1)").arg((int)capState);
        }
        m_captureStateDisplay->setText(capStr);
    }

    {
        auto previewMode = m_core->previewConfig().previewMode;
        QString previewModeStr;
        switch (previewMode) {
        case PmPreviewMode::Video: previewModeStr = "Video"; break;
        case PmPreviewMode::Region: previewModeStr = "Region"; break;
        case PmPreviewMode::MatchImage: previewModeStr = "Match Image"; break;
        default: previewModeStr = QString("Unknown (%1)").arg((int)previewMode);
        }
        m_previewModeDisplay->setText(previewModeStr);
    }
}
