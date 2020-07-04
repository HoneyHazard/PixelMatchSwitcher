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

using namespace std;

const QString PmMatchListWidget::k_dontSwitchStr 
    = obs_module_text("<don't switch>");
const QString PmMatchListWidget::k_transpBgStyle
    = "background-color: rgba(0, 0, 0, 0)";
const QString PmMatchListWidget::k_semiTranspBgStyle
    = "background-color: rgba(0, 0, 0, 0.6)";

PmMatchListWidget::PmMatchListWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    // table widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSortingEnabled(false);
    m_tableWidget->setColumnCount((int)RowOrder::NumRows);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() 
        << ""
        << obs_module_text("Match Config") 
        << obs_module_text("Match Scene")
        << obs_module_text("Transition")
        << obs_module_text("Result"));
    m_tableWidget->setStyleSheet(
        "QTableWidget::item { padding: 3px };"
        //"QTableWidget::item:selected:!active { selection-background-color: #3399ff }"
    );

    // config editing buttons
    m_cfgMoveUpBtn = new QPushButton(
        QIcon::fromTheme("go-up"), obs_module_text("Up"), this);
    m_cfgMoveUpBtn->setFocusPolicy(Qt::NoFocus);

    m_cfgMoveDownBtn = new QPushButton(
        QIcon::fromTheme("go-down"), obs_module_text("Down"), this);
    m_cfgMoveDownBtn->setFocusPolicy(Qt::NoFocus);

    m_cfgInsertBtn = new QPushButton(
        QIcon::fromTheme("list-add"), obs_module_text("Insert"), this);
    m_cfgInsertBtn->setFocusPolicy(Qt::NoFocus);

    m_cfgRemoveBtn = new QPushButton(
        QIcon::fromTheme("list-remove"), obs_module_text("Remove"), this);
    m_cfgRemoveBtn->setFocusPolicy(Qt::NoFocus);

    m_cfgClearBtn = new QPushButton(obs_module_text("Clear All"), this);
    m_cfgClearBtn->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(m_cfgMoveUpBtn);
    buttonsLayout->addWidget(m_cfgMoveDownBtn);
    buttonsLayout->addWidget(m_cfgInsertBtn);
    buttonsLayout->addWidget(m_cfgRemoveBtn);
    buttonsLayout->addWidget(m_cfgClearBtn);

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
    onNewMultiMatchConfigSize(cfgSz);

    onScenesChanged(m_core->scenes());

    for (size_t i = 0; i < multiConfig.size(); ++i) {
        onChangedMatchConfig(i, multiConfig[i]);
    }
    size_t selIdx = m_core->selectedConfigIndex();
    onSelectMatchIndex(
        selIdx, selIdx < cfgSz ? multiConfig[selIdx] : PmMatchConfig());

    onNoMatchSceneChanged(multiConfig.noMatchScene);
    onNoMatchTransitionChanged(multiConfig.noMatchTransition);

    auto multiResults = m_core->multiMatchResults();
    for (size_t i = 0; i < multiResults.size(); ++i) {
        onNewMatchResults(i, multiResults[i]);
    }

    // connections: core -> this
    connect(m_core, &PmCore::sigNewMatchResults,
        this, &PmMatchListWidget::onNewMatchResults, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMultiMatchConfigSize,
        this, &PmMatchListWidget::onNewMultiMatchConfigSize, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigChangedMatchConfig,
        this, &PmMatchListWidget::onChangedMatchConfig, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMatchResults,
        this, &PmMatchListWidget::onNewMatchResults, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigSelectMatchIndex,
        this, &PmMatchListWidget::onSelectMatchIndex, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigScenesChanged,
        this, &PmMatchListWidget::onScenesChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNoMatchSceneChanged,
        this, &PmMatchListWidget::onNoMatchSceneChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNoMatchTransitionChanged,
        this, &PmMatchListWidget::onNoMatchTransitionChanged);

    // connections: this -> core
    connect(this, &PmMatchListWidget::sigChangedMatchConfig,
        m_core, &PmCore::onChangedMatchConfig, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigSelectMatchIndex,
        m_core, &PmCore::onSelectMatchIndex, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigMoveMatchConfigUp,
        m_core, &PmCore::onMoveMatchConfigUp, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigMoveMatchConfigDown,
        m_core, &PmCore::onMoveMatchConfigDown, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigInsertMatchConfig,
        m_core, &PmCore::onInsertMatchConfig, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigRemoveMatchConfig,
        m_core, &PmCore::onRemoveMatchConfig, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigResetMatchConfigs,
        m_core, &PmCore::onResetMatchConfigs, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigNoMatchSceneChanged,
        m_core, &PmCore::onNoMatchSceneChanged, Qt::QueuedConnection);
    connect(this, &PmMatchListWidget::sigNoMatchTransitionChanged,
        m_core, &PmCore::onNoMatchTransitionChanged, Qt::QueuedConnection);

    // connections: local ui
    connect(m_tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &PmMatchListWidget::onRowSelected, Qt::QueuedConnection);
    connect(m_cfgMoveUpBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigMoveUpReleased, Qt::QueuedConnection);
    connect(m_cfgMoveDownBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigMoveDownReleased, Qt::QueuedConnection);
    connect(m_cfgInsertBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigInsertReleased, Qt::QueuedConnection);
    connect(m_cfgRemoveBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigRemoveReleased, Qt::QueuedConnection);
    connect(m_cfgClearBtn, &QPushButton::released,
        this, &PmMatchListWidget::onConfigClearReleased, Qt::QueuedConnection);
    connect(m_noMatchSceneCombo, &QComboBox::currentTextChanged,
        this, &PmMatchListWidget::onNoMatchSceneSelected, Qt::QueuedConnection);
    connect(m_noMatchTransitionCombo, &QComboBox::currentTextChanged,
        this, &PmMatchListWidget::onNoMatchTransitionSelected, Qt::QueuedConnection);
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

void PmMatchListWidget::onNewMultiMatchConfigSize(size_t sz)
{
    size_t oldSz = m_tableWidget->rowCount();
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
    if (oldSz == 0) {
        m_tableWidget->resizeColumnsToContents();
    }

    // enable/disable control buttons
    updateAvailableButtons((size_t)currentIndex(), sz);
}

void PmMatchListWidget::onChangedMatchConfig(size_t index, PmMatchConfig cfg)
{
    int idx = (int)index;

    auto enableBox = (QCheckBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::EnableBox);
    enableBox->blockSignals(true);
    enableBox->setChecked(cfg.filterCfg.is_enabled);
    enableBox->blockSignals(false);
    
    auto nameItem = m_tableWidget->item(idx, (int)RowOrder::ConfigName);
    nameItem->setText(cfg.label.data());
    nameItem->setToolTip(cfg.label.data());
    
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

    float percentage = results.percentageMatched;

    
    QString text;     
    if (percentage == percentage) { // not NaN
        text = QString("<font color=\"%1\">%2%</font>")
            .arg(results.isMatched ? "Green" : "Red")
            .arg(double(percentage), 0, 'f', 1);
    } else {
        text = obs_module_text("N/A");
    }
    resultLabel->setText(text);
    resultLabel->setToolTip(text);
}

void PmMatchListWidget::onSelectMatchIndex(
    size_t matchIndex, PmMatchConfig config)
{
    size_t mmSz = m_core->multiMatchConfigSize();
    if (m_prevMatchIndex < mmSz) {
        QLabel* prevLabel = (QLabel*)m_tableWidget->cellWidget(
            m_prevMatchIndex, int(RowOrder::Result));
        if (prevLabel) {
            prevLabel->setStyleSheet(k_transpBgStyle);
        }
    }
    if (matchIndex < mmSz) {
        QLabel* resLabel = (QLabel*)m_tableWidget->cellWidget(
            matchIndex, int(RowOrder::Result));
        if (resLabel) {
            resLabel->setStyleSheet(k_semiTranspBgStyle);
        }
    }
    m_prevMatchIndex = matchIndex;

    m_tableWidget->selectRow((int)matchIndex);

    updateAvailableButtons(matchIndex, mmSz);
}

void PmMatchListWidget::onRowSelected()
{
    int idx = currentIndex();
    if (idx == -1) {
        int prevIdx = (int)m_core->selectedConfigIndex();
        onSelectMatchIndex(prevIdx, m_core->matchConfig(prevIdx));
    } else {
        emit sigSelectMatchIndex(idx);
    }
}

void PmMatchListWidget::onConfigInsertReleased()
{
    int idx = currentIndex();
    PmMatchConfig newCfg = PmMatchConfig();
    newCfg.label = obs_module_text("new config");
    emit sigInsertMatchConfig(idx, newCfg);
    emit sigSelectMatchIndex(idx);
}

void PmMatchListWidget::onConfigRemoveReleased()
{
    int idx = currentIndex();
    emit sigRemoveMatchConfig(idx);
}

void PmMatchListWidget::onConfigMoveUpReleased()
{
    int idx = currentIndex();
    emit sigMoveMatchConfigUp(idx);
}

void PmMatchListWidget::onConfigMoveDownReleased()
{
    int idx = currentIndex();
    emit sigMoveMatchConfigDown(idx);
}

void PmMatchListWidget::onConfigClearReleased()
{
    emit sigResetMatchConfigs();
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

void PmMatchListWidget::constructRow(int idx)
{
    QWidget* parent = m_tableWidget;

    QCheckBox* enableBox = new QCheckBox(parent);
    enableBox->setStyleSheet(k_transpBgStyle);
    connect(enableBox, &QCheckBox::toggled,
        [this, idx](bool checked) { enableConfigToggled(idx, checked); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::EnableBox, enableBox);

    QString placeholderName = QString("placeholder %1").arg(idx);
#if 0
    QLineEdit* nameEdit = new QLineEdit(parent);
    nameEdit->setEnabled(false);
    nameEdit->setStyleSheet(bgStyle);
    nameEdit->setText(placeholderName);
    connect(nameEdit, &QLineEdit::textChanged,
        [this, idx](const QString& str) { configRenamed(idx, str); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::ConfigName, nameEdit);
#else
    m_tableWidget->setItem(
        idx, (int)RowOrder::ConfigName, new QTableWidgetItem(placeholderName));
#endif

    QComboBox* sceneCombo = new QComboBox(parent);
    sceneCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    sceneCombo->setStyleSheet(k_transpBgStyle);
    updateSceneChoices(sceneCombo);
    connect(sceneCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { matchSceneSelected(idx, str); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::SceneCombo, sceneCombo);

    QComboBox* transitionCombo = new QComboBox(parent);
    transitionCombo->setStyleSheet(k_transpBgStyle);
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
}

void PmMatchListWidget::updateAvailableButtons(
    size_t currIdx, size_t numConfigs)
{
    m_cfgMoveUpBtn->setEnabled(currIdx > 0 && currIdx < numConfigs);
    m_cfgMoveDownBtn->setEnabled(numConfigs > 0 && currIdx < numConfigs - 1);
    m_cfgRemoveBtn->setEnabled(currIdx < numConfigs);
    m_cfgClearBtn->setEnabled(numConfigs > 0);
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

void PmMatchListWidget::enableConfigToggled(int idx, bool enable)
{
    PmMatchConfig cfg = m_core->matchConfig(idx);
    cfg.filterCfg.is_enabled = enable;
    emit sigChangedMatchConfig(idx, cfg);

    m_tableWidget->selectRow(idx);
}

void PmMatchListWidget::matchSceneSelected(int idx, const QString& scene)
{
    PmMatchConfig cfg = m_core->matchConfig(idx);
    cfg.matchScene = (scene == k_dontSwitchStr) ? "" : scene.toUtf8().data();
    emit sigChangedMatchConfig(idx, cfg);
}

void PmMatchListWidget::matchTransitionSelected(int idx, const QString& name)
{
    PmMatchConfig cfg = m_core->matchConfig(idx);
    cfg.matchTransition = name.toUtf8().data();
    emit sigChangedMatchConfig(idx, cfg);
}


