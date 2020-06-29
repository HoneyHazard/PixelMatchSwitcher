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

const string PmMatchListWidget::k_dontSwitchStr 
    = obs_module_text("<don't switch>");

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
    m_tableWidget->setFocus();

    // config editing buttons
    m_cfgMoveUpBtn = new QPushButton(
        QIcon::fromTheme("go-up"), obs_module_text("Up"), this);
    m_cfgMoveDownBtn = new QPushButton(
        QIcon::fromTheme("go-down"), obs_module_text("Down"), this);
    m_cfgInsertBtn = new QPushButton(
        QIcon::fromTheme("list-add"), obs_module_text("Insert"), this);
    m_cfgRemoveBtn = new QPushButton(
        QIcon::fromTheme("list-remove"), obs_module_text("Remove"), this);
    m_cfgClearBtn = new QPushButton(obs_module_text("Clear All"), this);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(m_cfgMoveUpBtn);
    buttonsLayout->addWidget(m_cfgMoveDownBtn);
    buttonsLayout->addWidget(m_cfgInsertBtn);
    buttonsLayout->addWidget(m_cfgRemoveBtn);
    buttonsLayout->addWidget(m_cfgClearBtn);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(m_tableWidget);
    setLayout(mainLayout);

    // init state
    auto multiConfig = m_core->multiMatchConfig();
    onNewMultiMatchConfigSize(multiConfig.size());

    onScenesChanged(m_core->scenes());

    for (size_t i = 0; i < multiConfig.size(); ++i) {
        onChangedMatchConfig(i, multiConfig[i]);
    }
    size_t selIdx = m_core->selectedConfigIndex();
    onSelectMatchIndex(selIdx, m_core->matchConfig(selIdx));

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
    
    auto sceneCombo = (QComboBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::SceneCombo);
    sceneCombo->blockSignals(true);
    const string& newText
        = cfg.matchScene.size() ? cfg.matchScene : k_dontSwitchStr;
    sceneCombo->setCurrentText(newText.data());
    sceneCombo->blockSignals(false);

    auto transCombo = (QComboBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::TransitionCombo);
    transCombo->blockSignals(true);
    //int transIndex = transCombo->findText(cfg.matchTransition.data());
    //transCombo->setCurrentIndex(transIndex);
    transCombo->setCurrentText(cfg.matchTransition.data());
    transCombo->blockSignals(false);
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
}

void PmMatchListWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig config)
{
    m_tableWidget->selectRow((int)matchIndex);

    updateAvailableButtons(matchIndex, m_core->multiMatchConfigSize());
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

void PmMatchListWidget::constructRow(int idx)
{
    static const char* bgStyle = "background-color: rgba(0,0,0,0)";
    QWidget* parent = m_tableWidget;

    QCheckBox* enableBox = new QCheckBox(parent);
    enableBox->setStyleSheet(bgStyle);
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
    sceneCombo->setStyleSheet(bgStyle);
    updateSceneChoices(sceneCombo);
    connect(sceneCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { matchSceneSelected(idx, str); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::SceneCombo, sceneCombo);

    QComboBox* transitionCombo = new QComboBox(parent);
    transitionCombo->setStyleSheet(bgStyle);
    connect(transitionCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { transitionSelected(idx, str); });
    m_tableWidget->setCellWidget(
        idx, (int)RowOrder::TransitionCombo, transitionCombo);

    QLabel* resultLabel = new QLabel("--", parent);
    //resultLabel->setStyleSheet("background-color: rgba(0,0,0,1)");
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
        combo->addItem(val);
    }
    combo->addItem(k_dontSwitchStr.data());
    combo->setCurrentText(currText);
    combo->blockSignals(false);
}

int PmMatchListWidget::currentIndex() const
{
    return m_tableWidget->currentIndex().row();
}

void PmMatchListWidget::enableConfigToggled(int idx, bool enable)
{
    m_tableWidget->selectRow(idx);
}

#if 0
void PmMatchListWidget::configRenamed(int idx, const QString& name)
{
}
#endif

void PmMatchListWidget::matchSceneSelected(int idx, const QString& scene)
{
    PmMatchConfig cfg = m_core->matchConfig(idx);
    cfg.matchScene = scene.toUtf8().data();
    emit sigChangedMatchConfig(idx, cfg);
}

void PmMatchListWidget::transitionSelected(int idx, const QString& transition)
{
}

