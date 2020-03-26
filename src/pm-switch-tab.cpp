#include "pm-switch-tab.hpp"
#include "pm-core.hpp"

#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>

#include <obs-module.h>

PmSwitchTab::PmSwitchTab(PmCore *core, QWidget *parent)
: QWidget(parent)
, m_core(core)
{
    auto sceneConfig = m_core->switchConfig();

    QFormLayout *mainLayout = new QFormLayout;
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // enabled checkbox
    m_enabledCheck = new QCheckBox("", this);
    m_enabledCheck->setChecked(sceneConfig.isEnabled);
    connect(m_enabledCheck, &QCheckBox::stateChanged,
            this, &PmSwitchTab::onEnabledChanged, Qt::QueuedConnection);
    mainLayout->addRow(obs_module_text("Enabled: "), m_enabledCheck);

    // match scene combo
    m_matchSceneCombo = new QComboBox(this);
    m_matchSceneCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    connect(m_matchSceneCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onMatchSceneChanged()));
    mainLayout->addRow(
        obs_module_text("Match Activated Scene: "), m_matchSceneCombo);

    // no match scene combo
    m_noMatchSceneCombo = new QComboBox(this);
    m_noMatchSceneCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    connect(m_noMatchSceneCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onNoMatchSceneChanged()));
    mainLayout->addRow(
        obs_module_text("No Match Fallback Scene: "), m_noMatchSceneCombo);

    // finish layout
    setLayout(mainLayout);

    // connections with the core
    connect(m_core, &PmCore::sigScenesChanged,
            this, &PmSwitchTab::onScenesChanged, Qt::QueuedConnection);
    connect(this, &PmSwitchTab::sigSceneConfigChanged,
            m_core, &PmCore::onNewSceneConfig, Qt::QueuedConnection);

    // finish init
    onScenesChanged(m_core->scenes());
}

void PmSwitchTab::onScenesChanged(PmScenes scenes)
{
    m_matchSceneCombo->clear();
    m_noMatchSceneCombo->clear();

    auto sceneConfig = m_core->switchConfig();
    for (auto scene: scenes) {
        auto src = obs_weak_source_get_source(scene);
        auto name = obs_source_get_name(src);
        m_matchSceneCombo->addItem(name);
        m_noMatchSceneCombo->addItem(name);
    }

    OBSWeakSource matchScene = sceneConfig.matchScene;
    OBSWeakSource noMatchScene = sceneConfig.noMatchScene;
    if (!matchScene || !scenes.contains(matchScene)) {
        matchScene = pickScene(scenes, noMatchScene);
    }
    if (!noMatchScene || !scenes.contains(noMatchScene)
      || noMatchScene == matchScene) {
        noMatchScene = pickScene(scenes, matchScene);
    }

    setSelectedScene(m_matchSceneCombo, matchScene);
    setSelectedScene(m_noMatchSceneCombo, noMatchScene);

    if (sceneConfig.matchScene != matchScene
     || sceneConfig.noMatchScene != noMatchScene) {
        sceneConfig.matchScene = matchScene;
        sceneConfig.noMatchScene = noMatchScene;
        emit sigSceneConfigChanged(sceneConfig);
    }

}

void PmSwitchTab::onMatchSceneChanged()
{
    auto scenes = m_core->scenes();
    auto sc = m_core->switchConfig();
    sc.matchScene = findScene(scenes, m_matchSceneCombo->currentText());
    if (sc.matchScene && sc.noMatchScene == sc.matchScene) {
        sc.noMatchScene = pickScene(scenes, sc.matchScene);
        setSelectedScene(m_noMatchSceneCombo, sc.noMatchScene);
    }
    emit sigSceneConfigChanged(sc);
}

void PmSwitchTab::onNoMatchSceneChanged()
{
    auto sc = m_core->switchConfig();
    auto scenes = m_core->scenes();
    sc.noMatchScene = findScene(scenes, m_noMatchSceneCombo->currentText());
    if (sc.noMatchScene && sc.matchScene == sc.noMatchScene) {
        sc.matchScene = pickScene(scenes, sc.noMatchScene);
        setSelectedScene(m_matchSceneCombo, sc.matchScene);
    }
    emit sigSceneConfigChanged(sc);
}

void PmSwitchTab::onEnabledChanged()
{
    auto sc = m_core->switchConfig();
    sc.isEnabled = m_enabledCheck->isChecked();
    emit sigSceneConfigChanged(sc);
}

OBSWeakSource PmSwitchTab::pickScene(
    const PmScenes &scenes, const OBSWeakSource &another)
{
    for (auto &scene: scenes) {
        if (scene != another) {
            return scene;
        }
    }
    return nullptr;
}

void PmSwitchTab::setSelectedScene(QComboBox *combo, OBSWeakSource &scene)
{
    combo->blockSignals(true);
    if (!scene) {
        combo->setCurrentIndex(-1);
    } else {
        auto src = obs_weak_source_get_source(scene);
        const char* name = obs_source_get_name(src);
        combo->setCurrentText(name);
    }
    combo->blockSignals(false);
}

OBSWeakSource PmSwitchTab::findScene(const PmScenes &scenes, const QString &name)
{
    for (auto scene: scenes) {
        auto src = obs_weak_source_get_source(scene);
        if (obs_source_get_name(src) == name) {
            return scene;
        }
    }
    return nullptr;
}
