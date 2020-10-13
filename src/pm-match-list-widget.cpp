#include "pm-match-list-widget.hpp"
#include "pm-core.hpp"

#include <QTableWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QIcon>

#include <QDebug>

using namespace std;

const QString PmMatchListWidget::k_dontSwitchStr 
    = obs_module_text("<don't switch>");
const QString PmMatchListWidget::k_transpBgStyle
    = "background-color: rgba(0, 0, 0, 0)";
const QString PmMatchListWidget::k_semiTranspBgStyle
    = "background-color: rgba(0, 0, 0, 0.6)";

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
    m_tableWidget->setColumnCount((int)RowOrder::NumRows);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setResizeContentsPrecision(-1);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() 
        << obs_module_text("Enable")
        << obs_module_text("Match Config") 
        << obs_module_text("Target Scene")
        << obs_module_text("Transition")
        << obs_module_text("Result"));

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
    //buttonsLayout->addWidget(m_cfgClearBtn);

    // no-match configuration UI
    QLabel* noMatchSceneLabel = new QLabel(
        obs_module_text("No-match Scene: "), this);

    m_noMatchSceneCombo = new QComboBox(this);
    //m_noMatchSceneCombo->setInsertPolicy(QComboBox::InsertAlphabetically);

    QLabel* noMatchTransitionLabel = new QLabel(
        obs_module_text("No-match Transition: "), this);

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

    // top-level layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addLayout(noMatchLayout);
    setLayout(mainLayout);

    // init state
    auto multiConfig = m_core->multiMatchConfig();
    size_t cfgSz = multiConfig.size();
    onMultiMatchConfigSizeChanged(cfgSz);

    onScenesChanged(m_core->scenes());

    for (size_t i = 0; i < multiConfig.size(); ++i) {
        onMatchConfigChanged(i, multiConfig[i]);
    }
    size_t selIdx = m_core->selectedConfigIndex();
    onMatchConfigSelect(
        selIdx, selIdx < cfgSz ? multiConfig[selIdx] : PmMatchConfig());

    onNoMatchSceneChanged(multiConfig.noMatchScene);
    onNoMatchTransitionChanged(multiConfig.noMatchTransition);

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
    connect(m_core, &PmCore::sigNoMatchSceneChanged,
        this, &PmMatchListWidget::onNoMatchSceneChanged, qc);
    connect(m_core, &PmCore::sigNoMatchTransitionChanged,
        this, &PmMatchListWidget::onNoMatchTransitionChanged);

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
    connect(this, &PmMatchListWidget::sigNoMatchSceneChanged,
        m_core, &PmCore::onNoMatchSceneChanged, qc);
    connect(this, &PmMatchListWidget::sigNoMatchTransitionChanged,
        m_core, &PmCore::onNoMatchTransitionChanged, qc);

    // connections: local ui
    connect(m_tableWidget, &QTableWidget::itemChanged,
        this, &PmMatchListWidget::onItemChanged, qc);
    connect(m_tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &PmMatchListWidget::onRowSelected, qc);
    connect(m_cfgMoveUpBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigMoveUpButtonReleased, qc);
    connect(m_cfgMoveDownBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigMoveDownButtonReleased, qc);
    connect(m_cfgInsertBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigInsertButtonReleased, qc);
    connect(m_cfgRemoveBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigRemoveButtonReleased, qc);
    connect(m_noMatchSceneCombo, &QComboBox::currentTextChanged,
        this, &PmMatchListWidget::onNoMatchSceneSelected, qc);
    connect(m_noMatchTransitionCombo, &QComboBox::currentTextChanged,
        this, &PmMatchListWidget::onNoMatchTransitionSelected, qc);
}

void PmMatchListWidget::onScenesChanged(PmScenes scenes)
{
    m_sceneNames = scenes.sceneNames();

    int sz = (int)m_core->multiMatchConfigSize();
    for (int i = 0; i < sz; ++i) {
        QComboBox* sceneBox = (QComboBox*)m_tableWidget->cellWidget(
            i, (int)RowOrder::SceneCombo);
        updateSceneChoices(sceneBox);
    }

    updateSceneChoices(m_noMatchSceneCombo);
}

void PmMatchListWidget::onMultiMatchConfigSizeChanged(size_t sz)
{
    size_t oldSz = size_t(m_tableWidget->rowCount());
    m_tableWidget->setRowCount((int)sz + 1);
    // widgets in the new rows are constructed, when necessary
    for (size_t i = oldSz ? oldSz-1 : 0; i < sz; ++i) {
        constructRow((int)i);
    }
    // last row below is empty (for insertion)
    for (int c = 0; c < (int)RowOrder::NumRows; ++c) {
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
        idx, (int)RowOrder::EnableBox);
    enableBox->blockSignals(true);
    enableBox->setChecked(cfg.filterCfg.is_enabled);
    enableBox->blockSignals(false);
    
    auto nameItem = m_tableWidget->item(idx, (int)RowOrder::ConfigName);
    m_tableWidget->blockSignals(true);
    nameItem->setText(cfg.label.data());
    nameItem->setToolTip(cfg.label.data());
    m_tableWidget->blockSignals(false);

    auto sceneCombo = (QComboBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::SceneCombo);
    sceneCombo->blockSignals(true);
    if (cfg.matchScene.size()) {
        sceneCombo->setCurrentText(cfg.matchScene.data());
    } else {
        sceneCombo->setCurrentText(k_dontSwitchStr);
    }
    sceneCombo->setToolTip(sceneCombo->currentText());
    sceneCombo->blockSignals(false);

    auto transCombo = (QComboBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::TransitionCombo);
    transCombo->blockSignals(true);
    transCombo->setCurrentText(cfg.matchTransition.data());
    transCombo->setToolTip(transCombo->currentText());
    transCombo->blockSignals(false);
}

void PmMatchListWidget::onNoMatchSceneChanged(std::string sceneName)
{
    m_noMatchSceneCombo->blockSignals(true);
    if (sceneName.size()) {
        m_noMatchSceneCombo->setCurrentText(sceneName.data());
    } else {
        m_noMatchSceneCombo->setCurrentText(k_dontSwitchStr);
    }
    m_noMatchSceneCombo->setToolTip(m_noMatchSceneCombo->currentText());
    m_noMatchSceneCombo->blockSignals(false);
}

void PmMatchListWidget::onNoMatchTransitionChanged(std::string transName)
{
    m_noMatchTransitionCombo->blockSignals(true);
    m_noMatchTransitionCombo->setCurrentText(transName.data());
    m_noMatchTransitionCombo->setToolTip(
        m_noMatchTransitionCombo->currentText());
    m_noMatchTransitionCombo->blockSignals(false);
}

void PmMatchListWidget::onNewMatchResults(size_t index, PmMatchResults results)
{
    int idx = (int)index;

    auto resultLabel = (QLabel*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::Result);
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
            m_prevMatchIndex, int(RowOrder::Result));
        if (prevLabel) {
            prevLabel->setStyleSheet(k_transpBgStyle);
        }
    }
    if (matchIndex < mmSz) {
        QLabel* resLabel = (QLabel*)m_tableWidget->cellWidget(
            int(matchIndex), int(RowOrder::Result));
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
    size_t idx = (size_t)(currentIndex());
    PmMatchConfig newCfg = PmMatchConfig();
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

void PmMatchListWidget::onNoMatchSceneSelected(QString scene)
{
    std::string noMatchScene = 
        (scene == k_dontSwitchStr) ? "" : scene.toUtf8().data();
    emit sigNoMatchSceneChanged(noMatchScene);
}

void PmMatchListWidget::onNoMatchTransitionSelected(QString str)
{
    emit sigNoMatchTransitionChanged(str.toUtf8().data());
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

    QCheckBox* enableBox = new QCheckBox(parent);
    enableBox->setStyleSheet(k_transpBgStyle);
    connect(enableBox, &QCheckBox::toggled,
        [this, idx](bool checked) { enableConfigToggled(idx, checked); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::EnableBox, enableBox);

    QString placeholderName = QString("placeholder %1").arg(idx);
    auto labelItem = new QTableWidgetItem(placeholderName);
    labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
    m_tableWidget->setItem(
        idx, (int)RowOrder::ConfigName, labelItem);

    QComboBox* sceneCombo = new QComboBox(parent);
    sceneCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    sceneCombo->setStyleSheet(k_transpBgStyle);
    sceneCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    updateSceneChoices(sceneCombo);
    connect(sceneCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { matchSceneSelected(idx, str); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::SceneCombo, sceneCombo);

    QComboBox* transitionCombo = new QComboBox(parent);
    transitionCombo->setStyleSheet(k_transpBgStyle);
    transitionCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    updateTransitionChoices(transitionCombo);
    connect(transitionCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { matchTransitionSelected(idx, str); });
    m_tableWidget->setCellWidget(
        idx, (int)RowOrder::TransitionCombo, transitionCombo);

    QLabel* resultLabel = new QLabel("--", parent);
    resultLabel->setStyleSheet(k_transpBgStyle);
    resultLabel->setTextFormat(Qt::RichText);
    resultLabel->setAlignment(Qt::AlignCenter);
    m_tableWidget->setCellWidget(
        idx, (int)RowOrder::Result, resultLabel);

    // do this every time a row is added; otherwise new rows dont look correct
    m_tableWidget->setStyleSheet("QTableWidget::item { padding: 1px };");
}

void PmMatchListWidget::updateAvailableButtons(
    size_t currIdx, size_t numConfigs)
{
    m_cfgMoveUpBtn->setEnabled(currIdx > 0 && currIdx < numConfigs);
    m_cfgMoveDownBtn->setEnabled(numConfigs > 0 && currIdx < numConfigs - 1);
    m_cfgRemoveBtn->setEnabled(currIdx < numConfigs);
    //m_cfgClearBtn->setEnabled(numConfigs > 0);
}

void PmMatchListWidget::updateSceneChoices(
    QComboBox* combo)
{
    combo->blockSignals(true);
    auto currText = combo->currentText();
    combo->clear();
    for (const auto& val : m_sceneNames) {
        combo->addItem(val.data());
    }
    combo->addItem(k_dontSwitchStr);
    combo->setCurrentText(currText);
    combo->setToolTip(combo->currentText());
    combo->blockSignals(false);
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

int PmMatchListWidget::currentIndex() const
{
    return m_tableWidget->currentIndex().row();
}

void PmMatchListWidget::onItemChanged(QTableWidgetItem* item)
{
    if (item->column() != (int)RowOrder::ConfigName) return;

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

    m_tableWidget->selectRow(idx);
}

void PmMatchListWidget::matchSceneSelected(int idx, const QString& scene)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    cfg.matchScene = (scene == k_dontSwitchStr) ? "" : scene.toUtf8().data();
    emit sigMatchConfigChanged(index, cfg);
}

void PmMatchListWidget::matchTransitionSelected(int idx, const QString& name)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    cfg.matchTransition = name.toUtf8().data();
    emit sigMatchConfigChanged(index, cfg);
}


