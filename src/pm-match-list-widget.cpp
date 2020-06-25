#include "pm-match-list-widget.hpp"
#include "pm-core.hpp"

#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

using namespace std;

PmMatchListWidget::PmMatchListWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSortingEnabled(false);
    m_tableWidget->setStyleSheet("QTableWidget::item { padding: 3px }");
    m_tableWidget->setColumnCount(5);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() 
        << ""
        << obs_module_text("Match Config") 
        << obs_module_text("Match Scene")
        << obs_module_text("Transition")
        << obs_module_text("Status"));

    m_tableWidget->setRowCount(2);
    constructRow(0);
    constructRow(1);

    m_tableWidget->resizeColumnsToContents();

    QVBoxLayout* mainLayout = new QVBoxLayout;
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

void PmMatchListWidget::onChangedMatchConfig(size_t idx, PmMatchConfig cfg)
{
}

void PmMatchListWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig config)
{
}

void PmMatchListWidget::onRowSelected()
{
    int idx = m_tableWidget->currentIndex().row();
    emit sigSelectMatchIndex(idx);
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

#if 0
    QLineEdit* nameEdit = new QLineEdit(parent);
    nameEdit->setEnabled(false);
    nameEdit->setStyleSheet(bgStyle);
    nameEdit->setText();
    connect(nameEdit, &QLineEdit::textChanged,
        [this, idx](const QString& str) { configRenamed(idx, str); });
    m_tableWidget->setCellWidget(idx, (int)RowOrder::ConfigName, nameEdit);
#endif

    QString name = QString("placeholder %1").arg(idx);
    m_tableWidget->setItem(idx, (int)RowOrder::ConfigName, new QTableWidgetItem(name));

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

void PmMatchListWidget::enableConfigToggled(size_t idx, bool enable)
{
    m_tableWidget->selectRow(idx);
}

void PmMatchListWidget::configRenamed(size_t idx, const QString& name)
{
}

void PmMatchListWidget::matchSceneSelected(size_t idx, const QString& scene)
{
}

void PmMatchListWidget::transitionSelected(size_t idx, const QString& transition)
{
}

