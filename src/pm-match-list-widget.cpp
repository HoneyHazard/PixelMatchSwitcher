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

PmMatchListWidget::PmMatchListWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    // table widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSortingEnabled(false);
    m_tableWidget->setColumnCount((int)RowOrder::NumRows);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() 
        << ""
        << obs_module_text("Match Config") 
        << obs_module_text("Match Scene")
        << obs_module_text("Transition")
        << obs_module_text("Status"));
    m_tableWidget->setStyleSheet(
        "QTableWidget::item { padding: 3px };"
        //"QTableWidget::item:selected:!active { selection-background-color: #3399ff }"
    );


    // config editing buttons
    m_cfgMoveUpBtn = new QPushButton(
        QIcon::fromTheme("go-up"), obs_module_text("Up"), this);
    m_cfgMoveDownBtn = new QPushButton(
        QIcon::fromTheme("go-down"), obs_module_text("Down"), this);
    m_cfgInsertBtn = new QPushButton(
        QIcon::fromTheme("list-add"), obs_module_text("Insert"), this);
    m_cfgRemoveBtn = new QPushButton(
        QIcon::fromTheme("list-remove"), obs_module_text("Remove"), this);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(m_cfgMoveUpBtn);
    buttonsLayout->addWidget(m_cfgMoveDownBtn);
    buttonsLayout->addWidget(m_cfgInsertBtn);
    buttonsLayout->addWidget(m_cfgRemoveBtn);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(m_tableWidget);
    setLayout(mainLayout);

    // init state
    auto multiConfig = m_core->multiMatchConfig();
    onNewMultiMatchConfigSize(multiConfig.size());
    for (size_t i = 0; i < multiConfig.size(); ++i) {
        onChangedMatchConfig(i, multiConfig[i]);
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
    connect(m_core, &PmCore::sigChangedMatchConfig,
        this, &PmMatchListWidget::onChangedMatchConfig, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMatchResults,
        this, &PmMatchListWidget::onNewMatchResults, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigSelectMatchIndex,
        this, &PmMatchListWidget::onSelectMatchIndex, Qt::QueuedConnection);

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
}

void PmMatchListWidget::onScenesChanged(PmScenes scenes)
{

}

void PmMatchListWidget::onNewMatchResults(size_t idx, PmMatchResults results)
{
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
}

void PmMatchListWidget::onChangedMatchConfig(size_t index, PmMatchConfig cfg)
{
    int idx = (int)index;

    auto enableBox = (QCheckBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::EnableBox);
    enableBox->blockSignals(true);
    enableBox->setChecked(cfg.isEnabled);
    enableBox->blockSignals(false);
    
    auto nameItem = m_tableWidget->item(idx, (int)RowOrder::ConfigName);
    nameItem->setText(cfg.label.data());
    
    auto sceneCombo = (QComboBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::SceneCombo);
    sceneCombo->blockSignals(true);
    int sceneIndex = sceneCombo->findText(cfg.matchScene.data());
    sceneCombo->setCurrentIndex(sceneIndex);
    sceneCombo->blockSignals(false);

    auto transCombo = (QComboBox*)m_tableWidget->cellWidget(
        idx, (int)RowOrder::TransitionCombo);
    transCombo->blockSignals(true);
    int transIndex = transCombo->findText(cfg.matchTransition.data());
    transCombo->setCurrentIndex(transIndex);
    transCombo->blockSignals(false);
}

void PmMatchListWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig config)
{
    m_tableWidget->selectRow((int)matchIndex);
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
}

void PmMatchListWidget::onConfigMoveDownReleased()
{
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
    connect(sceneCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { matchSceneSelected(idx, str); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::SceneCombo, sceneCombo);

    QComboBox* transitionCombo = new QComboBox(parent);
    transitionCombo->setStyleSheet(bgStyle);
    connect(transitionCombo, &QComboBox::currentTextChanged,
        [this, idx](const QString& str) { transitionSelected(idx, str); });
    m_tableWidget->setCellWidget(
        idx, (int)RowOrder::TransitionCombo, transitionCombo);
}

int PmMatchListWidget::currentIndex() const
{
    return m_tableWidget->currentIndex().row();
}

void PmMatchListWidget::enableConfigToggled(int idx, bool enable)
{
    m_tableWidget->selectRow(idx);
}

void PmMatchListWidget::configRenamed(int idx, const QString& name)
{
}

void PmMatchListWidget::matchSceneSelected(int idx, const QString& scene)
{
}

void PmMatchListWidget::transitionSelected(int idx, const QString& transition)
{
}

