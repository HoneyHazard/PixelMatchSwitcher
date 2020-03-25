#include "pixel-match-scenes-tab.hpp"
#include "pixel-match-core.hpp"

#include <QFormLayout>
#include <QComboBox>

#include <obs-module.h>

PmScenesTab::PmScenesTab(PmCore *core, QWidget *parent)
: QWidget(parent)
, m_core(core)
{
    QFormLayout *mainLayout = new QFormLayout;
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // match scene combo
    m_matchSceneCombo = new QComboBox(this);
    connect(m_matchSceneCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onConfigUiChanged()));
    mainLayout->addRow(
        obs_module_text("Match Activated Scene: "), m_matchSceneCombo);

    // no match scene combo
    m_noMatchSceneCombo = new QComboBox(this);
    connect(m_noMatchSceneCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onConfigUiChanged()));
    mainLayout->addRow(
        obs_module_text("No Match Fallback Scene: "), m_noMatchSceneCombo);

    // finish layout
    setLayout(mainLayout);

    // connections with the core
    connect(m_core, &PmCore::sigScenesChanged,
            this, &PmScenesTab::onScenesChanged, Qt::QueuedConnection);
    connect(this, &PmScenesTab::sigSceneConfigChanged,
            m_core, &PmCore::onNewSceneConfig, Qt::QueuedConnection);
}

void PmScenesTab::onScenesChanged(PmScenes scenes)
{

}

void PmScenesTab::onConfigUiChanged()
{

}
