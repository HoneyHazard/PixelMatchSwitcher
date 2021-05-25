#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"
#include "pm-core.hpp"

#include <obs-module.h>

#include <QComboBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default>");

PmActionEntryWidget::PmActionEntryWidget(size_t actionIndex, QWidget *parent)
: QWidget(parent)
, m_actionIndex(actionIndex)
{
    m_actionTypeCombo = new QComboBox(this);
    m_actionTypeCombo->addItem(
        obs_module_text("<don't switch>"), int(PmActionType::None));
    m_actionTypeCombo->addItem(
        obs_module_text("Scene"), int(PmActionType::Scene));
    m_actionTypeCombo->addItem(
        obs_module_text("Filter"), int(PmActionType::Filter));
    m_actionTypeCombo->addItem(
        obs_module_text("Hotkey"), int(PmActionType::Hotkey));
    m_actionTypeCombo->addItem(
        obs_module_text("FrontEndEvent"), int(PmActionType::FrontEndEvent));

    m_targetCombo = new QComboBox(this);

    m_transitionsCombo = new QComboBox(this);
    m_detailsStack->addWidget(m_transitionsCombo);

    m_toggleCombo = new QComboBox(this);
    m_toggleCombo->addItem(obs_module_text("Show"), int(PmToggleCode::Show));
    m_toggleCombo->addItem(obs_module_text("Hide"), int(PmToggleCode::Hide));
    m_detailsStack->addWidget(m_toggleCombo);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_actionTypeCombo);
    mainLayout->addWidget(m_targetCombo);
    mainLayout->addWidget(m_detailsStack);
    setLayout(mainLayout);
}

void PmActionEntryWidget::updateScenes(const QList<std::string> &scenes)
{
    // TODO: color
    PmActionType actionType = PmActionType(
        int(m_actionTypeCombo->currentData().toInt()));
    if (actionType != PmActionType::Scene) return;

    m_targetCombo->clear();
    for (const std::string& scene : scenes) {
        m_targetCombo->addItem(scene.data());
    }
}

void PmActionEntryWidget::updateSceneItems(const QList<std::string> &sceneItems)
{
    // TODO: color
    PmActionType actionType = PmActionType(
        int(m_actionTypeCombo->currentData().toInt()));
    if (actionType != PmActionType::SceneItem) return;

    m_targetCombo->clear();
    for (const std::string& sceneItem : sceneItems) {
        m_targetCombo->addItem(sceneItem.data());
    }
}

void PmActionEntryWidget::updateFilters(const QList<std::string> &sceneFilters)
{
    // TODO: color; group by scene items
    PmActionType actionType = PmActionType(
        int(m_actionTypeCombo->currentData().toInt()));
    if (actionType != PmActionType::Filter) return;

    m_targetCombo->clear();
    for (const std::string& filter : sceneFilters) {
        m_targetCombo->addItem(filter.data());
    }
}

void PmActionEntryWidget::updateTransitons(
    const QList<std::string> &transitions)
{
    m_transitionsCombo->clear();
    m_transitionsCombo->addItem(k_defaultTransitionStr, "");
    for (const std::string &t : transitions) {
        m_transitionsCombo->addItem(t.data(), t.data());
    }
}

void PmActionEntryWidget::updateSizeHints(QList<QSize> &columnSizes)
{
    QList<QSize> sizes = {
        m_actionTypeCombo->sizeHint(),
        m_targetCombo->sizeHint(),
        m_detailsStack->sizeHint()
    };
    while (columnSizes.size() < 3) {
        columnSizes.append({0, 0});
    }
    for (int i = 0; i < columnSizes.size(); ++i) {
        columnSizes[i] = {
            qMax(columnSizes[i].width(), sizes[i].width()),
            qMax(columnSizes[i].height(), sizes[i].height())
        };
    }
}


void PmActionEntryWidget::updateAction(size_t actionIndex, PmAction action)
{
    int findIdx = m_actionTypeCombo->findData(int(action.m_actionCode));
    m_actionTypeCombo->setCurrentIndex(findIdx);

    switch (action.m_actionType) {
    case PmActionType::None:
        m_targetCombo->setVisible(false);
        break;
    case PmActionType::Scene:
        m_targetCombo->setVisible(true);
        m_targetCombo->setCurrentText(action.m_targetElement.data());
        m_detailsStack->setCurrentWidget(m_transitionsCombo);
        m_transitionsCombo->setCurrentText(action.m_targetDetails.data());
        break;
    case PmActionType::SceneItem:
        m_targetCombo->setVisible(true);
        m_targetCombo->setCurrentText(action.m_targetElement.data());
        m_detailsStack->setCurrentWidget(m_toggleCombo);
        findIdx = m_toggleCombo->findData(action.m_actionCode);
        m_toggleCombo->setCurrentIndex(findIdx);
        break;
    case PmActionType::Filter:
        m_targetCombo->setVisible(true);
        m_targetCombo->setCurrentText(action.m_targetElement.data());
        m_detailsStack->setCurrentWidget(m_toggleCombo);
        findIdx = m_toggleCombo->findData(action.m_actionCode);
        m_toggleCombo->setCurrentIndex(findIdx);
        break;
    }
}

void PmActionEntryWidget::onUiChanged()
{
    PmAction action;
    action.m_actionCode = int(m_actionTypeCombo->currentData().toInt());

    switch (PmActionType(action.m_actionCode)) {
    case PmActionType::None:
        break;
    case PmActionType::Scene:
        action.m_targetElement = m_targetCombo->currentText().toUtf8().data();
        action.m_targetDetails = std::string(
            m_transitionsCombo->currentText().toUtf8().data());
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        action.m_targetElement = m_targetCombo->currentText().toUtf8().data();
        action.m_actionCode = int(m_toggleCombo->currentData().toInt());
        break;
    }

    emit sigActionChanged(m_actionIndex, action);
}

//----------------------------------------------------

PmMatchReactionWidget::PmMatchReactionWidget(
    PmCore *core,
    ReactionTarget reactionTarget, ReactionType reactionType,
    QWidget *parent)
: QGroupBox(parent)
, m_core(core)
, m_reactionTarget(reactionTarget)
, m_reactionType(reactionType)
{
    m_insertActionButton = prepareButton(obs_module_text("Insert New Match Entry"),
        ":/res/images/add.png", "addIconSmall");
    m_removeActionButton = prepareButton(obs_module_text("Remove Match Entry"),
        ":/res/images/list_remove.png",
        "removeIconSmall");
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_insertActionButton);
    buttonLayout->addWidget(m_removeActionButton);

    m_actionLayout = new QVBoxLayout();

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(m_actionLayout);
    setLayout(mainLayout);

    //--------------------------------------

    const auto qc = Qt::QueuedConnection;

    if (reactionTarget == Entry) {
        connect(m_core, &PmCore::sigMatchConfigSelect,
                this, &PmMatchReactionWidget::onMatchConfigSelect, qc);
        connect(m_core, &PmCore::sigMatchConfigChanged,
                this, &PmMatchReactionWidget::onMatchConfigChanged, qc);
        connect(m_core, &PmCore::sigMultiMatchConfigSizeChanged,
                this, &PmMatchReactionWidget::onMultiMatchConfigSizeChanged, qc);

        onMultiMatchConfigSizeChanged(m_core->multiMatchConfigSize());
        size_t idx = m_core->selectedConfigIndex();
        onMatchConfigSelect(idx, m_core->matchConfig(idx));
    } else { // Anything
        connect(m_core, &PmCore::sigNoMatchReactionChanged,
                this, &PmMatchReactionWidget::onNoMatchReactionChanged, qc);
        connect(this, &PmMatchReactionWidget::sigNoMatchReactionChanged,
                m_core, &PmCore::onNoMatchReactionChanged, qc);
        setTitle((m_reactionType == Match)
            ? obs_module_text("Anything Matched")
            : obs_module_text("Nothing Matched")); 
    }
}

void PmMatchReactionWidget::onMatchConfigChanged(
    size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    setTitle(QString(obs_module_text("%1 Actions #%2: %3"))
        .arg((m_reactionType == Match) ?
                obs_module_text("Match") : obs_module_text("Unmatch"))
        .arg(matchIdx + 1)
        .arg(cfg.label.data()));

    const PmReaction &reaction = cfg.reaction;
    updateReaction(reaction);
}

void PmMatchReactionWidget::onNoMatchReactionChanged(PmReaction reaction)
{
    updateReaction(reaction);
}

void PmMatchReactionWidget::updateReaction(const PmReaction &reaction)
{
    const auto &actionList = (m_reactionType == Match) ?
        reaction.matchActions : reaction.unmatchActions;

    size_t listSz = reaction.matchSz();
    for (size_t i = 0; i < listSz; ++i) {
        PmActionEntryWidget *entryWidget = nullptr;
        if (i > m_actionWidgets.size()) {
            PmActionEntryWidget *entryWidget = new PmActionEntryWidget(i, this);
            connect(entryWidget, &PmActionEntryWidget::sigActionChanged,
                    this, &PmMatchReactionWidget::onActionChanged);

            m_actionWidgets.push_back(entryWidget);
            m_actionLayout->addWidget(entryWidget);
        } else {
            entryWidget = m_actionWidgets[i];
        }
        entryWidget->updateAction(i, actionList[i]);
    }
    for (size_t i = m_actionWidgets.size(); i < listSz; ++i) {
        PmActionEntryWidget *entryWidget = m_actionWidgets[i];
        m_actionLayout->removeWidget(entryWidget);
        entryWidget->deleteLater();
    }
    m_actionWidgets.resize(listSz);
}

void PmMatchReactionWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;

    setEnabled(matchIndex < m_multiConfigSz);

    onMatchConfigChanged(matchIndex, cfg);
}

void PmMatchReactionWidget::onMultiMatchConfigSizeChanged(size_t sz)
{
    m_multiConfigSz = sz;
    setEnabled(m_matchIndex < m_multiConfigSz);
}

void PmMatchReactionWidget::onScenesChanged(
    QList<std::string> scenes, QList<std::string> sceneItems)
{
    QList<std::string> filterNames;
    for (const std::string siName : sceneItems) {
        filterNames.append(m_core->filters(siName));
    }

    for (auto *w : m_actionWidgets) {
        w->updateScenes(scenes);
        w->updateSceneItems(sceneItems);
        w->updateFilters(filterNames);
    }
}

void PmMatchReactionWidget::onActionChanged(size_t actionIndex, PmAction action)
{
    PmReaction reaction = (m_reactionType == Entry) ?
        m_core->reaction(m_matchIndex) : m_core->noMatchReaction();


    if (m_reactionType == Match) {
        reaction.matchActions[actionIndex] = action;
    } else {
        reaction.unmatchActions[actionIndex] = action;
    }

    if (m_reactionTarget == Entry) {
        PmMatchConfig cfg = m_core->matchConfig(m_matchIndex);
        cfg.reaction = reaction;
        emit sigMatchConfigChanged(m_matchIndex, cfg);
    } else {
        emit sigNoMatchReactionChanged(reaction);
    }
}

QPushButton *PmMatchReactionWidget::prepareButton(const char *tooltip,
                          const char *icoPath,
                          const char *themeId)
{
    QIcon icon;
    icon.addFile(icoPath, QSize(), QIcon::Normal, QIcon::Off);

    QPushButton *ret = new QPushButton(icon, "", this);
    ret->setToolTip(tooltip);
    ret->setIcon(icon);
    ret->setIconSize(QSize(16, 16));
    ret->setMaximumSize(22, 22);
    ret->setFlat(true);
    ret->setProperty("themeID", QVariant(themeId));
    ret->setFocusPolicy(Qt::NoFocus);

    return ret;
}
