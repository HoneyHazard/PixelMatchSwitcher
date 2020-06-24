#include "pm-match-list-widget.hpp"
#include "pm-core.hpp"

#include <QListWidget>
#include <QVBoxLayout>

using namespace std;

PmMatchListWidget::PmMatchListWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    m_listWidget = new QListWidget(this);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_listWidget);
    setLayout(mainLayout);

    // init state
    auto multiConfig = m_core->multiMatchConfig();
    onNewMultiMatchConfigSize(multiConfig.size());
    for (size_t i = 0; i < multiConfig.size(); ++i) {
        onNewMatchConfig(i, multiConfig[i]);
    }

    auto multiResults = m_core->multiMatchResults();
    for (size_t i = 0; i < multiResults.size(); ++i) {
        onNewMatchResults(i, multiResults[i]);
    }

    // connections: core -> this
    connect(m_core, &PmCore::sigNewMatchResults,
        this, &PmMatchListWidget::onNewMatchResults, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMultiMatchConfigSize,
        this, &PmMatchListWidget::onNewMultiMatchConfigSize, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMatchResults,
        this, &PmMatchListWidget::onNewMatchResults, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigSelectMatchIndex,
        this, &PmMatchListWidget::onSelectMatchIndex, Qt::QueuedConnection);

    // connections: this -> core
    connect(this, &PmMatchListWidget::sigNewMatchConfig,
        m_core, &PmCore::onNewMatchConfig, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigSelectMatchIndex,
        m_core, &PmCore::onSelectMatchIndex, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigMoveMatchConfigUp,
        m_core, &PmCore::onMoveMatchConfigUp, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigMoveMatchConfigDown,
        m_core, &PmCore::onMoveMatchConfigDown, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigInsertMatchConfig,
        m_core, &PmCore::onInsertMatchConfig, Qt::QueuedConnection);
}

void PmMatchListWidget::onScenesChanged(PmScenes scenes)
{

}

void PmMatchListWidget::onNewMatchResults(size_t idx, PmMatchResults results)
{
}

void PmMatchListWidget::onNewMultiMatchConfigSize(size_t sz)
{
}

void PmMatchListWidget::onNewMatchConfig(size_t idx, PmMatchConfig cfg)
{
}

void PmMatchListWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig config)
{
}
