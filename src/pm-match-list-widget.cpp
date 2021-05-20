#include "pm-match-list-widget.hpp"
#include "pm-core.hpp"

#include <QTableWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QIcon>
#include <QStandardItemModel>

#include <QMouseEvent>
#include <QTimer> // for QTimer::singleShot()

#include <QDebug>

using namespace std;

enum class PmMatchListWidget::ColOrder : int {
    EnableBox = 0,
    ConfigName = 1,
    Reaction = 2,
    ReactionDisplay = 3,
    Result = 4,
    NumCols = 5,
};

enum class PmMatchListWidget::ActionStackOrder : int {
    SceneTarget = 0,
    SceneItemTarget = 1
};

const QStringList PmMatchListWidget::k_columnLabels = {
    obs_module_text("Enable"),
    obs_module_text("Label"),
    obs_module_text("Target"),
    obs_module_text("Action"),
    obs_module_text("Linger ms"),
    obs_module_text("Result")
};

const QString PmMatchListWidget::k_dontSwitchStr 
    = obs_module_text("<don't switch>");
const QString PmMatchListWidget::k_scenesLabelStr
    = obs_module_text("--- Scenes ---");
const QString PmMatchListWidget::k_sceneItemsLabelStr
    = obs_module_text("--- Scene Items & Filters ---");
const QString PmMatchListWidget::k_showLabelStr = obs_module_text("Show");
const QString PmMatchListWidget::k_hideLabelStr = obs_module_text("Hide");

const QColor PmMatchListWidget::k_dontSwitchColor = Qt::lightGray;
const QColor PmMatchListWidget::k_scenesLabelColor = Qt::darkCyan;
const QColor PmMatchListWidget::k_scenesColor = Qt::cyan;
const QColor PmMatchListWidget::k_sceneItemsLabelColor = Qt::darkYellow;
const QColor PmMatchListWidget::k_sceneItemsColor = Qt::yellow;

const QString PmMatchListWidget::k_transpBgStyle
    = "background-color: rgba(0, 0, 0, 0);";
const QString PmMatchListWidget::k_semiTranspBgStyle
    = "background-color: rgba(0, 0, 0, 0.6);";

PmMatchListWidget::PmMatchListWidget(PmCore* core, QWidget* parent)
: QGroupBox(obs_module_text("Active Match Entries"), parent)
, m_core(core)
{
    // table widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked
                                 | QAbstractItemView::EditKeyPressed
                                 | QAbstractItemView::SelectedClicked);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSortingEnabled(false);
    m_tableWidget->setColumnCount((int)ColOrder::NumCols);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    m_tableWidget->setHorizontalHeaderLabels(k_columnLabels);

    // config editing buttons
    m_cfgInsertBtn = prepareButton(obs_module_text("Insert New Match Entry"),
        ":/res/images/add.png", "addIconSmall");
    m_cfgMoveUpBtn = prepareButton(obs_module_text("Move Match Entry Up"),
        ":/res/images/up.png", "upArrowIconSmall");
    m_cfgMoveDownBtn = prepareButton(obs_module_text("Move Match Entry Down"),
        ":/res/images/down.png", "downArrowIconSmall");
    m_cfgRemoveBtn = prepareButton(obs_module_text("Remove Match Entry"),
        ":/res/images/list_remove.png", "removeIconSmall");

    QSpacerItem* buttonSpacer1 = new QSpacerItem(0, 0, QSizePolicy::Expanding);
    QSpacerItem* buttonSpacer2 = new QSpacerItem(0, 0, QSizePolicy::Expanding);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(m_cfgInsertBtn);
    buttonsLayout->addItem(buttonSpacer1);
    buttonsLayout->addWidget(m_cfgMoveUpBtn);
    buttonsLayout->addWidget(m_cfgMoveDownBtn);
    buttonsLayout->addItem(buttonSpacer2);
    buttonsLayout->addWidget(m_cfgRemoveBtn);

    // no-match configuration UI
    QLabel* noMatchSceneLabel = new QLabel(
        obs_module_text("No-match Scene: "), this);

    //m_noMatchSceneCombo = new QComboBox(this);
    //m_noMatchSceneCombo->setInsertPolicy(QComboBox::InsertAlphabetically);

    QLabel* noMatchTransitionLabel = new QLabel(
        obs_module_text("No-match Transition: "), this);

#if 0
    m_noMatchTransitionCombo = new QComboBox(this);
    updateTransitionChoices(m_noMatchTransitionCombo);

    QFrame* spacer = new QFrame();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout* noMatchLayout = new QHBoxLayout();
    noMatchLayout->addWidget(noMatchSceneLabel);
    noMatchLayout->addWidget(m_noMatchSceneCombo);
    noMatchLayout->addWidget(spacer);
    noMatchLayout->addWidget(noMatchTransitionLabel);
    noMatchLayout->addWidget(m_noMatchTransitionCombo);
#endif

    // top-level layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(m_tableWidget);
    //mainLayout->addLayout(noMatchLayout);
    setLayout(mainLayout);

    // init state
    auto multiConfig = m_core->multiMatchConfig();
    size_t cfgSz = multiConfig.size();
    onMultiMatchConfigSizeChanged(cfgSz);

    onScenesChanged(m_core->sceneNames(), m_core->sceneItemNames());

    for (size_t i = 0; i < multiConfig.size(); ++i) {
        onMatchConfigChanged(i, multiConfig[i]);
    }
    size_t selIdx = m_core->selectedConfigIndex();
    onMatchConfigSelect(
        selIdx, selIdx < cfgSz ? multiConfig[selIdx] : PmMatchConfig());

    //onNoMatchReactionChanged(multiConfig.noMatchReaction);

    auto multiResults = m_core->multiMatchResults();
    for (size_t i = 0; i < multiResults.size(); ++i) {
        onNewMatchResults(i, multiResults[i]);
    }
    
    const Qt::ConnectionType qc = Qt::QueuedConnection;

    // connections: core -> this
    connect(m_core, &PmCore::sigNewMatchResults,
        this, &PmMatchListWidget::onNewMatchResults, qc);
    connect(m_core, &PmCore::sigMultiMatchConfigSizeChanged,
        this, &PmMatchListWidget::onMultiMatchConfigSizeChanged, qc);
    connect(m_core, &PmCore::sigMatchConfigChanged,
        this, &PmMatchListWidget::onMatchConfigChanged, qc);
    connect(m_core, &PmCore::sigNewMatchResults,
        this, &PmMatchListWidget::onNewMatchResults, qc);
    connect(m_core, &PmCore::sigMatchConfigSelect,
        this, &PmMatchListWidget::onMatchConfigSelect, qc);
    connect(m_core, &PmCore::sigScenesChanged,
        this, &PmMatchListWidget::onScenesChanged, qc);
    //connect(m_core, &PmCore::sigNoMatchReactionChanged,
    //    this, &PmMatchListWidget::onNoMatchReactionChanged, qc);

    // connections: this -> core
    connect(this, &PmMatchListWidget::sigMatchConfigChanged,
        m_core, &PmCore::onMatchConfigChanged, qc);
    connect(this, &PmMatchListWidget::sigMatchConfigSelect,
        m_core, &PmCore::onMatchConfigSelect, qc);
    connect(this, &PmMatchListWidget::sigMatchConfigMoveUp,
        m_core, &PmCore::onMatchConfigMoveUp, qc);
    connect(this, &PmMatchListWidget::sigMatchConfigMoveDown,
        m_core, &PmCore::onMatchConfigMoveDown, qc);
    connect(this, &PmMatchListWidget::sigMatchConfigInsert,
        m_core, &PmCore::onMatchConfigInsert, qc);
    connect(this, &PmMatchListWidget::sigMatchConfigRemove,
        m_core, &PmCore::onMatchConfigRemove, qc);
    //connect(this, &PmMatchListWidget::sigNoMatchReactionChanged,
    //    m_core, &PmCore::onNoMatchReactionChanged, qc);

    // connections: local ui
    connect(m_tableWidget, &QTableWidget::itemChanged,
        this, &PmMatchListWidget::onItemChanged, qc);
    connect(m_tableWidget->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this, &PmMatchListWidget::onRowSelected, qc);
    connect(m_tableWidget, &QTableWidget::cellChanged,
        this, &PmMatchListWidget::onCellChanged, qc);
    connect(m_cfgMoveUpBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigMoveUpButtonReleased, qc);
    connect(m_cfgMoveDownBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigMoveDownButtonReleased, qc);
    connect(m_cfgInsertBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigInsertButtonReleased, qc);
    connect(m_cfgRemoveBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigRemoveButtonReleased, qc);
    //connect(m_noMatchSceneCombo, &QComboBox::currentTextChanged,
    //    this, &PmMatchListWidget::onNoMatchSceneSelected, qc);
    //connect(m_noMatchTransitionCombo, &QComboBox::currentTextChanged,
    //    this, &PmMatchListWidget::onNoMatchTransitionSelected, qc);
}

#if 0
void PmMatchListWidget::onScenesChanged(
    QList<std::string> scenes, QList<std::string> sceneItems)
{
    int sz = (int)m_core->multiMatchConfigSize();
    for (int i = 0; i < sz; ++i) {
        QComboBox* sceneBox = (QComboBox*)m_tableWidget->cellWidget(
            i, (int)ColOrder::TargetSelect);
        updateTargetChoices(sceneBox, scenes, sceneItems);
    }

    updateTargetChoices(m_noMatchSceneCombo, scenes, {});
}
#endif

void PmMatchListWidget::onMultiMatchConfigSizeChanged(size_t sz)
{
    size_t oldSz = size_t(m_tableWidget->rowCount());
    m_tableWidget->setRowCount((int)sz + 1);
    // widgets in the new rows are constructed, when necessary
    for (size_t i = oldSz ? oldSz-1 : 0; i < sz; ++i) {
        constructRow((int)i);
    }
    // last row below is empty (for insertion)
    for (int c = 0; c < (int)ColOrder::NumCols; ++c) {
        m_tableWidget->setCellWidget((int)sz, c, nullptr);
        m_tableWidget->setItem((int)sz, c, nullptr);
    }

    // enable/disable control buttons
    updateAvailableButtons((size_t)currentIndex(), sz);

    // workaround for a buggy behavior that automatic resizing isn't handling
    m_tableWidget->resizeColumnsToContents();
}

void PmMatchListWidget::onMatchConfigChanged(size_t index, PmMatchConfig cfg)
{
    int idx = (int)index;

    auto enableBox = (QCheckBox*)m_tableWidget->cellWidget(
        idx, (int)ColOrder::EnableBox);
    if (enableBox) {
        enableBox->blockSignals(true);
        enableBox->setChecked(cfg.filterCfg.is_enabled);
        enableBox->blockSignals(false);
    }

    auto nameItem = m_tableWidget->item(idx, (int)ColOrder::ConfigName);
    if (nameItem) {
        m_tableWidget->blockSignals(true);
        nameItem->setText(cfg.label.data());
        nameItem->setToolTip(cfg.label.data());
        m_tableWidget->blockSignals(false);
    }

#if 0
    const PmReactionOld &reaction = cfg.reactionOld;
    auto targetCombo = (QComboBox*)m_tableWidget->cellWidget(
        idx, (int)ColOrder::TargetSelect);
    if (targetCombo) {
        updateTargetSelection(targetCombo, reaction, true);
    }

    auto actionStack = (QStackedWidget*)m_tableWidget->cellWidget(
        idx, (int)ColOrder::ActionSelect);
    if (actionStack) {
        if (reaction.type == PmActionType::SwitchScene) {
            int idx = int(ActionStackOrder::SceneTarget);
            actionStack->setCurrentIndex(idx);
            auto sceneTransCombo = (QComboBox *)actionStack->widget(idx);
            actionStack->setCurrentWidget(sceneTransCombo);
            if (sceneTransCombo) {
                sceneTransCombo->blockSignals(true);
                sceneTransCombo->setCurrentText(
                    reaction.sceneTransition.data());
                sceneTransCombo->setToolTip(
                    sceneTransCombo->currentText());
                sceneTransCombo->blockSignals(false);
            }
        } else {
            int idx = int(ActionStackOrder::SceneItemTarget);
            actionStack->setCurrentIndex(idx);
            auto sceneItemActionCombo = (QComboBox* )actionStack->widget(idx);
            if (sceneItemActionCombo) {
                sceneItemActionCombo->blockSignals(true);
                bool show = reaction.type == PmActionType::ShowSceneItem
                         || reaction.type == PmActionType::ShowFilter;
                QString selStr = show ? k_showLabelStr : k_hideLabelStr;
                sceneItemActionCombo->setCurrentText(selStr);
                sceneItemActionCombo->blockSignals(false);
            }
        }
    }

    auto lingerDelayBox = (QSpinBox *)m_tableWidget->cellWidget(
        idx, (int)ColOrder::LingerDelay);
    if (lingerDelayBox) {
        lingerDelayBox->blockSignals(true);
        lingerDelayBox->setValue(reaction.lingerMs);
        lingerDelayBox->blockSignals(false);
    }

#endif


    setMinWidth();

    // enable/disable control buttons
    updateAvailableButtons(
        (size_t)currentIndex(), m_core->multiMatchConfigSize());
}

#if 0
void PmMatchListWidget::onNoMatchReactionChanged(PmReaction noMatchReaction)
{
    updateTargetSelection(m_noMatchSceneCombo, noMatchReaction);

    m_noMatchTransitionCombo->blockSignals(true);
    m_noMatchTransitionCombo->setCurrentText(
        noMatchReaction.sceneTransition.data());
    m_noMatchTransitionCombo->setToolTip(
        m_noMatchTransitionCombo->currentText());
    m_noMatchTransitionCombo->blockSignals(false);
}
#endif

void PmMatchListWidget::onNewMatchResults(size_t index, PmMatchResults results)
{
    int idx = (int)index;

    auto resultLabel = (QLabel*)m_tableWidget->cellWidget(
        idx, (int)ColOrder::Result);
    if (!resultLabel) return;

    float percentage = results.percentageMatched;
    
    QString text;     
    if (percentage == percentage && results.numCompared > 0) { // valid
        text = QString("<font color=\"%1\">%2%</font>")
            .arg(results.isMatched ? "Green" : "Red")
            .arg(double(percentage), 0, 'f', 1);
    } else {
        text = obs_module_text("N/A");
    }
    resultLabel->setText(text);
    resultLabel->setToolTip(text);
}

void PmMatchListWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig config)
{
    size_t mmSz = m_core->multiMatchConfigSize();
    if (size_t(m_prevMatchIndex) < mmSz) {
        QLabel* prevLabel = (QLabel*)m_tableWidget->cellWidget(
            m_prevMatchIndex, int(ColOrder::Result));
        if (prevLabel) {
            prevLabel->setStyleSheet(k_transpBgStyle);
        }
    }
    if (matchIndex < mmSz) {
        QLabel* resLabel = (QLabel*)m_tableWidget->cellWidget(
            int(matchIndex), int(ColOrder::Result));
        if (resLabel) {
            resLabel->setStyleSheet(k_semiTranspBgStyle);
        }
    }
    m_prevMatchIndex = int(matchIndex);

    m_tableWidget->selectRow((int)matchIndex);

    updateAvailableButtons(matchIndex, mmSz);

    UNUSED_PARAMETER(config);
}

void PmMatchListWidget::onRowSelected()
{
    int idx = currentIndex();
    if (idx == -1) {
        size_t prevIdx = (size_t)(m_core->selectedConfigIndex());
        onMatchConfigSelect(prevIdx, m_core->matchConfig(prevIdx));
    } else {
        emit sigMatchConfigSelect(size_t(idx));
    }
}

void PmMatchListWidget::onConfigInsertButtonReleased()
{
    PmMatchConfig newCfg;

    size_t idx = (size_t)(currentIndex());

#if 0
    size_t sz = m_core->multiMatchConfigSize();
    if (sz > 0) {
        size_t closestIdx = std::min(idx, sz-1);
        PmReaction closestReaction = m_core->reaction(closestIdx);
        newCfg.reactionOld.type = closestReaction.type;
    }
#endif

    emit sigMatchConfigInsert(idx, newCfg);
    emit sigMatchConfigSelect(idx);
}

void PmMatchListWidget::onConfigRemoveButtonReleased()
{
    size_t idx = (size_t)(currentIndex());
    emit sigMatchConfigRemove(idx);
}

void PmMatchListWidget::onConfigMoveUpButtonReleased()
{
    size_t idx = (size_t)(currentIndex());
    emit sigMatchConfigMoveUp(idx);
}

void PmMatchListWidget::onConfigMoveDownButtonReleased()
{
    size_t idx = (size_t)(currentIndex());
    emit sigMatchConfigMoveDown(idx);
}

#if 0
void PmMatchListWidget::onNoMatchSceneSelected(QString scene)
{
    std::string noMatchScene = 
        (scene == k_dontSwitchStr) ? "" : scene.toUtf8().data();

    
    PmReaction noMatchReaction = m_core->noMatchReaction();
    noMatchReaction.targetScene = noMatchScene;
    emit sigNoMatchReactionChanged(noMatchReaction);
}

void PmMatchListWidget::onNoMatchTransitionSelected(QString str)
{
    PmReaction noMatchReaction = m_core->noMatchReaction();
    noMatchReaction.sceneTransition = str.toUtf8().data();
    emit sigNoMatchReactionChanged(noMatchReaction);
}
#endif

void PmMatchListWidget::onCellChanged(int row, int col)
{
    int numRows = m_tableWidget->rowCount();
    if (row == numRows - 1) {
        m_tableWidget->setItem(row, col, nullptr);
    }
}

QPushButton* PmMatchListWidget::prepareButton(
    const char *tooltip, const char* icoPath, const char* themeId)
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

void PmMatchListWidget::constructRow(int idx)
{
    QWidget* parent = m_tableWidget;

    // toggle match config on or off
    QCheckBox* enableBox = new QCheckBox(parent);
    enableBox->setStyleSheet(k_transpBgStyle);
    connect(enableBox, &QCheckBox::toggled,
        [this, idx](bool checked) { enableConfigToggled(idx, checked); });
    enableBox->installEventFilter(this);
    m_tableWidget->setCellWidget(idx, (int)ColOrder::EnableBox, enableBox);

    // config label edit
    QString placeholderName = QString("placeholder %1").arg(idx);
    auto labelItem = new QTableWidgetItem(placeholderName);
    labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
    m_tableWidget->setItem(
        idx, (int)ColOrder::ConfigName, labelItem);

    #if 0
    // target scene or scene item
    QComboBox* targetCombo = new QComboBox(parent);
    targetCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    targetCombo->setStyleSheet(k_transpBgStyle);
    targetCombo->setAttribute(Qt::WA_TranslucentBackground);
    targetCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    updateTargetChoices(targetCombo, scenes, sceneItems);
    targetCombo->installEventFilter(this);
    connect(targetCombo, &QComboBox::currentTextChanged,
        [this, targetCombo, idx](){ targetSelected(idx, targetCombo); });
    m_tableWidget->setCellWidget(idx, (int)ColOrder::TargetSelect, targetCombo);

    // scene transition or scene item action
    QComboBox* sceneTransitionCombo = new QComboBox(parent);
    sceneTransitionCombo->setStyleSheet(k_transpBgStyle);
    updateTransitionChoices(sceneTransitionCombo);
    sceneTransitionCombo->installEventFilter(this);
    connect(sceneTransitionCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { sceneTransitionSelected(idx, str); });

    QComboBox *sceneItemActionCombo = new QComboBox(parent);
    sceneItemActionCombo->setStyleSheet(k_transpBgStyle);
    sceneItemActionCombo->addItem(k_showLabelStr);
    sceneItemActionCombo->addItem(k_hideLabelStr);
    sceneItemActionCombo->installEventFilter(this);
    connect(sceneItemActionCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString &str) { sceneItemActionSelected(idx, str); });

    QStackedWidget *targetActionStack = new QStackedWidget(parent);
    targetActionStack->setStyleSheet(
        QString("QWidget { %1 }").arg(k_transpBgStyle));
    targetActionStack->setContentsMargins(0, 0, 0, 0);
    targetActionStack->addWidget(sceneTransitionCombo);
    targetActionStack->addWidget(sceneItemActionCombo);
    m_tableWidget->setCellWidget(
        idx, (int)ColOrder::ActionSelect, targetActionStack);

    // linger delay
    QSpinBox *lingerDelayBox = new QSpinBox();
    lingerDelayBox->setStyleSheet(k_transpBgStyle);
    lingerDelayBox->setRange(0, 10000);
    lingerDelayBox->setSingleStep(10);
    lingerDelayBox->installEventFilter(this);
    void (QSpinBox::*sigLingerValueChanged)(int value)
        = &QSpinBox::valueChanged;
    connect(lingerDelayBox, sigLingerValueChanged,
        [this, idx](int val) { lingerDelayChanged(idx, val); });
    m_tableWidget->setCellWidget(
        idx, (int)ColOrder::LingerDelay, lingerDelayBox);
    #endif

    PmReactionLabel *reactionLabel = new PmReactionLabel("--", parent);
    reactionLabel->setStyleSheet(k_transpBgStyle);
    reactionLabel->setTextFormat(Qt::RichText);
    reactionLabel->installEventFilter(this);
    m_tableWidget->setCellWidget(
        idx, (int)ColOrder::Reaction, reactionLabel);

    // result
    QLabel *resultLabel = new PmResultsLabel("--", parent);
    resultLabel->setStyleSheet(k_transpBgStyle);
    resultLabel->setTextFormat(Qt::RichText);
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->installEventFilter(this);
    m_tableWidget->setCellWidget(
        idx, (int)ColOrder::Result, resultLabel);

    // do this every time a row is added; otherwise new rows dont look correct
    m_tableWidget->setStyleSheet("QTableWidget::item { padding: 2px };");
}

void PmMatchListWidget::updateAvailableButtons(
    size_t currIdx, size_t numConfigs)
{
    m_cfgMoveUpBtn->setEnabled(m_core->matchConfigCanMoveUp(currIdx));
    m_cfgMoveDownBtn->setEnabled(m_core->matchConfigCanMoveDown(currIdx));
    m_cfgRemoveBtn->setEnabled(currIdx < numConfigs);
    //m_cfgClearBtn->setEnabled(numConfigs > 0);
}

#if 0
void PmMatchListWidget::updateTargetChoices(QComboBox* combo,
    const QList<std::string> &scenes, const QList<std::string> &sceneItems)
{
    if (!combo) return;

    auto model = dynamic_cast<QStandardItemModel *>(combo->model());

    combo->blockSignals(true);
    auto currText = combo->currentText();
    combo->clear();

    QFont mainFont = font();
    mainFont.setBold(true);

    int idx = 0;

    combo->addItem(k_dontSwitchStr);
    combo->setItemData(idx, "", Qt::UserRole);
    combo->setItemData(idx, mainFont, Qt::FontRole);
    combo->setItemData(idx, QBrush(k_dontSwitchColor), Qt::TextColorRole);
    idx++;

    combo->addItem(k_scenesLabelStr);
    combo->setItemData(idx, QBrush(k_scenesLabelColor), Qt::TextColorRole);
    model->item(idx)->setEnabled(false);
    idx++;

    for (const std::string &sceneName : scenes) {
        combo->addItem(sceneName.data());
        combo->setItemData(idx, sceneName.data(), Qt::UserRole);
        combo->setItemData(idx, mainFont, Qt::FontRole);
        combo->setItemData(idx, QBrush(k_scenesColor), Qt::TextColorRole);
        idx++;
    }

    if (sceneItems.size()) {
        combo->addItem(k_sceneItemsLabelStr);
        combo->setItemData(
            idx, QBrush(k_sceneItemsLabelColor), Qt::TextColorRole);
        model->item(idx)->setEnabled(false);
        idx++;

        QFont filtersFont = font();
        filtersFont.setBold(false);

        for (const std::string &siName : sceneItems) {
            combo->addItem(siName.data());
            combo->setItemData(idx, siName.data(), Qt::UserRole);
            combo->setItemData(idx, mainFont, Qt::FontRole);
            combo->setItemData(
                idx, QBrush(k_sceneItemsColor), Qt::TextColorRole);
            idx++;

            QList<std::string> filterNames = m_core->filters(siName);
            for (const std::string& fiName : filterNames) {
                QString fiLabel
                    = QString(" ") + QChar(0x25CF) + ' ' + fiName.data();
                combo->addItem(fiLabel);
                combo->setItemData(idx, fiName.data(), Qt::UserRole);
                combo->setItemData(
                    idx, QBrush(k_sceneItemsColor), Qt::TextColorRole);
                combo->setItemData(idx, filtersFont, Qt::FontRole);
                idx++;
            }
        }
    }

    combo->setCurrentText(currText);
    combo->setToolTip(combo->currentText());
    combo->blockSignals(false);
}

void PmMatchListWidget::updateTargetSelection(
    QComboBox *targetCombo, const PmReactionOld &reaction, bool transparent)
{
    QColor targetColor;
    std::string targetStr;
    if (reaction.isSet()) {
        if (reaction.type == PmActionType::SwitchScene) {
            targetStr = reaction.targetScene;
            targetColor = k_scenesColor;
        } else if (reaction.type == PmActionType::ShowSceneItem
                || reaction.type == PmActionType::HideSceneItem) {
            targetStr = reaction.targetSceneItem;
            targetColor = k_sceneItemsColor;
        } else {
            targetStr = reaction.targetFilter;
            targetColor = k_sceneItemsColor;
        }
    } else {
        targetColor = k_dontSwitchColor;
        targetStr = "";
    }
    QString stylesheet = QString("%1; color: %2")
        .arg(transparent ? k_transpBgStyle : "")
        .arg(targetColor.name());

    targetCombo->blockSignals(true);
    int idx = targetCombo->findData(targetStr.data(), Qt::UserRole);
    targetCombo->setCurrentIndex(idx);
    targetCombo->setStyleSheet(stylesheet);
    targetCombo->setToolTip(targetCombo->currentText());
    targetCombo->blockSignals(false);
}

void PmMatchListWidget::updateTransitionChoices(QComboBox* combo)
{
    combo->blockSignals(true);
    auto currText = combo->currentText();
    combo->clear();
    auto availableTransitions = m_core->availableTransitions();
    for (const auto& val : availableTransitions) {
        combo->addItem(val.data());
    }
    combo->setCurrentText(currText);
    combo->setToolTip(combo->currentText());
    combo->blockSignals(false);
}
#endif

int PmMatchListWidget::currentIndex() const
{
    return m_tableWidget->currentIndex().row();
}

void PmMatchListWidget::onItemChanged(QTableWidgetItem* item)
{
    if (item->column() != (int)ColOrder::ConfigName) return;

    size_t matchIndex = (size_t)item->row();
    PmMatchConfig cfg = m_core->matchConfig(matchIndex);
    cfg.label = item->text().toUtf8().data();
    emit sigMatchConfigChanged(matchIndex, cfg);
}

void PmMatchListWidget::enableConfigToggled(int idx, bool enable)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    cfg.filterCfg.is_enabled = enable;
    emit sigMatchConfigChanged(index, cfg);
}

#if 0
void PmMatchListWidget::targetSelected(int matchIdx, QComboBox *box)
{
    size_t index = (size_t)(matchIdx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    PmReactionOld &reaction = cfg.reactionOld;
    QByteArray ba = box->currentData(Qt::UserRole).toByteArray();
    std::string targetStr(ba.data());

    if (targetStr.empty()) {
        reaction = PmReactionOld();
    } else {
        QList<std::string> sceneNames = m_core->sceneNames();
        QList<std::string> sceneItemNames = m_core->sceneItemNames();
        if (sceneNames.contains(targetStr)) {
            reaction.type = PmActionType::SwitchScene;
            reaction.targetScene = targetStr;
            reaction.targetSceneItem.clear();
            reaction.targetFilter.clear();
        } else if (sceneItemNames.contains(targetStr)) {
            if (reaction.type == PmActionType::HideSceneItem
             || reaction.type == PmActionType::HideFilter) {
                reaction.type = PmActionType::HideSceneItem;
            } else {
                reaction.type = PmActionType::ShowSceneItem;
            }
            reaction.targetSceneItem = targetStr;
            reaction.targetScene.clear();
            reaction.targetFilter.clear();
        } else {
            if (reaction.type == PmActionType::HideSceneItem
             || reaction.type == PmActionType::HideFilter) {
                reaction.type = PmActionType::HideFilter;
            } else {
                reaction.type = PmActionType::ShowFilter;
            }
            reaction.targetFilter = targetStr;
            reaction.targetScene.clear();
            reaction.targetSceneItem.clear();
        }
    }

    emit sigMatchConfigChanged(index, cfg);
}

void PmMatchListWidget::sceneTransitionSelected(
    int idx, const QString& transStr)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    cfg.reactionOld.sceneTransition = transStr.toUtf8().data();
    emit sigMatchConfigChanged(index, cfg);
}

void PmMatchListWidget::sceneItemActionSelected(
    int idx, const QString &actionStr)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    if (actionStr == k_showLabelStr) {
        cfg.reactionOld.type = PmActionType::ShowSceneItem;
    } else if (actionStr == k_hideLabelStr) {
        cfg.reactionOld.type = PmActionType::HideSceneItem;
    }
    emit sigMatchConfigChanged(index, cfg);
}

void PmMatchListWidget::lingerDelayChanged(int idx, int lingerMs)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    cfg.reactionOld.lingerMs = lingerMs;
    emit sigMatchConfigChanged(index, cfg);
}
#endif

void PmMatchListWidget::setMinWidth()
{
    int width = m_tableWidget->horizontalHeader()->length() + 20;
    m_tableWidget->setMinimumWidth(width);
}

bool PmMatchListWidget::selectRowAtGlobalPos(QPoint globalPos)
{
    auto tablePos = m_tableWidget->mapFromGlobal(globalPos);
    tablePos.setY(tablePos.y() -
              m_tableWidget->horizontalHeader()->height());
    auto tableIndex = m_tableWidget->indexAt(tablePos);
    if (m_core->selectedConfigIndex() != (size_t)tableIndex.row()) {
        m_tableWidget->selectRow(tableIndex.row());
        return true;
    } else {
        return false;
    }
}

bool PmMatchListWidget::eventFilter(QObject *obj, QEvent *event)
{
    // filter events for all of the cell widgets
    if (event->type() == QEvent::FocusIn) {
        QSpinBox* spinBox = dynamic_cast<QSpinBox *>(obj);
        if (spinBox) {
            // helps an active box look right in the middle of a highlited row
            QPoint globalPos = spinBox->mapToGlobal(QPoint(1, 1));
            if (selectRowAtGlobalPos(globalPos)) {
                // reactivate focus after table selection causes focus loss
                QTimer::singleShot(50, [spinBox]() {
                    spinBox->setFocus();
                    spinBox->setStyleSheet("");
                });
            } else {
                // row already selected. just update background
                spinBox->setStyleSheet("");
            }
        }
    } else if (event->type() == QEvent::FocusOut) {
        // helps the spin box
        auto spinBox = dynamic_cast<QSpinBox *>(obj);
        if (spinBox)
            spinBox->setStyleSheet(k_transpBgStyle);
    } else if (event->type() == QEvent::MouseButtonPress) {
        // clicking combo boxes and other elements should select their row
        auto mouseEvent = (QMouseEvent *)event;
        selectRowAtGlobalPos(mouseEvent->globalPos());
    }

    // do not interfere with events
    return QObject::eventFilter(obj, event);
}

PmResultsLabel::PmResultsLabel(const QString &text, QWidget *parentWidget)
    : QLabel(text, parentWidget)
{
    QFontMetrics fm(font());
    auto m = contentsMargins();
    m_resultTextWidth = fm.width("100.0%")
        + m.left() + m.right() + margin() * 2 + 10;
};

QSize PmResultsLabel::sizeHint() const
{
    QSize ret = QLabel::sizeHint();
    ret.setWidth(m_resultTextWidth);
    return ret;
}

PmReactionLabel::PmReactionLabel(const QString &text, QWidget *parent)
: QLabel(parent)
{
    // TODO
}

void PmReactionLabel::updateReaction(const PmReaction &reaction)
{
    // TODO
}

QSize PmReactionLabel::sizeHint() const
{
    return QSize();
}
