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
#include <QScrollBar>

#include <QMouseEvent>
#include <QTimer> // for QTimer::singleShot()

#include <QDebug>

using namespace std;

enum class PmMatchListWidget::ColOrder : int {
    EnableBox = 0,
    ConfigName = 1,
    MatchActions = 2,
    UnmatchActions = 3,
    Linger = 4,
    Cooldown = 5,
    Result = 6,
    NumCols = 7,
};

enum class PmMatchListWidget::ActionStackOrder : int {
    SceneTarget = 0,
    SceneItemTarget = 1
};

const QStringList PmMatchListWidget::k_columnLabels = {
    obs_module_text("Enable"),
    obs_module_text("Label"),
    obs_module_text("On Match"),
    obs_module_text("On Unmatch"),
    obs_module_text("Linger"),
    obs_module_text("Cooldown"),
    obs_module_text("Result")
};

const QString PmMatchListWidget::k_transpBgStyle
    = "background-color: rgba(0, 0, 0, 0);";
const QString PmMatchListWidget::k_semiTranspBgStyle
    = "background-color: rgba(0, 0, 0, 0.6);";

PmMatchListWidget::PmMatchListWidget(PmCore* core, QWidget* parent)
: PmSpoilerWidget(parent)
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
    m_tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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

    m_buttonSpacer1 = new QSpacerItem(20, 0);
    m_buttonSpacer2 = new QSpacerItem(20, 0);

    m_buttonsLayout = new QHBoxLayout;
    m_buttonsLayout->addWidget(m_cfgInsertBtn);
    m_buttonsLayout->addItem(m_buttonSpacer1);
    m_buttonsLayout->addWidget(m_cfgMoveUpBtn);
    m_buttonsLayout->addWidget(m_cfgMoveDownBtn);
    m_buttonsLayout->addItem(m_buttonSpacer2);
    m_buttonsLayout->addWidget(m_cfgRemoveBtn);
    m_buttonsLayout->setContentsMargins(0, 0, 0, 0);
    setTopRightLayout(m_buttonsLayout);

    // top-level layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_tableWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setContentLayout(mainLayout);
    m_contentArea->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Expanding);

    // init state
    auto multiConfig = m_core->multiMatchConfig();
    size_t cfgSz = multiConfig.size();
    onMultiMatchConfigSizeChanged(cfgSz);

    for (size_t i = 0; i < multiConfig.size(); ++i) {
        onMatchConfigChanged(i, multiConfig[i]);
    }
    size_t selIdx = m_core->selectedConfigIndex();
    onMatchConfigSelect(
        selIdx, selIdx < cfgSz ? multiConfig[selIdx] : PmMatchConfig());

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
    for (int c = 0; c < (int)ColOrder::NumCols; ++c) {
        m_tableWidget->setCellWidget((int)sz, c, nullptr);
        m_tableWidget->setItem((int)sz, c, nullptr);
    }

    // enable/disable control buttons
    updateButtonsState();

    // workaround for a buggy behavior that automatic resizing isn't handling
    m_tableWidget->resizeColumnsToContents();
    updateContentHeight();

    QString title = obs_module_text("Active Match Entries");
    if (sz > 0)
	    title += QString(" [%1]").arg(sz);
    setTitle(title);
}

void PmMatchListWidget::onMatchConfigChanged(size_t index, PmMatchConfig cfg)
{
    int idx = (int)index;

    int rowHeight = 0;

    auto enableBox = (QCheckBox*)m_tableWidget->cellWidget(
        idx, (int)ColOrder::EnableBox);
    if (enableBox) {
        enableBox->blockSignals(true);
        enableBox->setChecked(cfg.filterCfg.is_enabled);
        enableBox->blockSignals(false);
    	rowHeight = qMax(rowHeight, enableBox->sizeHint().height());
    }

    auto nameItem = m_tableWidget->item(idx, (int)ColOrder::ConfigName);
    if (nameItem) {
        m_tableWidget->blockSignals(true);
        nameItem->setText(cfg.label.data());
        nameItem->setToolTip(cfg.label.data());
        m_tableWidget->blockSignals(false);
        rowHeight = qMax(rowHeight, nameItem->sizeHint().height());
    }

    const PmReaction &reaction = cfg.reaction;

    PmReactionDisplay *matchDisplay = (PmReactionDisplay *)
        m_tableWidget->cellWidget(idx, (int)ColOrder::MatchActions);
    if (matchDisplay) {
	    matchDisplay->updateReaction(reaction, PmReactionType::Match);
	    rowHeight = qMax(rowHeight, matchDisplay->sizeHint().height());
    }

    PmReactionDisplay *unmatchDisplay = (PmReactionDisplay *)
        m_tableWidget->cellWidget(idx, (int)ColOrder::UnmatchActions);
    if (unmatchDisplay) {
	    unmatchDisplay->updateReaction(reaction, PmReactionType::Unmatch);
	    rowHeight
            = qMax(rowHeight, unmatchDisplay->sizeHint().height());
    }

    PmContainerWidget *lingerContainer = (PmContainerWidget *)
        m_tableWidget->cellWidget(idx, (int)ColOrder::Linger);
    QSpinBox* lingerDelayBox = (QSpinBox *)lingerContainer->containedWidget();
    if (lingerDelayBox) {
        lingerDelayBox->blockSignals(true);
        lingerDelayBox->setValue(reaction.lingerMs);
        lingerDelayBox->blockSignals(false);
    	rowHeight
            = qMax(rowHeight, lingerDelayBox->sizeHint().height());
    }

    PmContainerWidget *cooldownContainer = (PmContainerWidget *)
        m_tableWidget->cellWidget(idx, (int)ColOrder::Cooldown);
    QSpinBox *cooldownDelayBox
        = (QSpinBox *)cooldownContainer->containedWidget();
    if (cooldownDelayBox) {
        cooldownDelayBox->blockSignals(true);
        cooldownDelayBox->setValue(reaction.cooldownMs);
        cooldownDelayBox->blockSignals(false);
	    rowHeight
            = qMax(rowHeight, cooldownDelayBox->sizeHint().height());
    }

    // control horizontal sizing
    m_tableWidget->resizeColumnsToContents();
    setMinWidth();

    // control vertical sizing
    rowHeight += k_rowPadding; // padding
    m_tableWidget->verticalHeader()->resizeSection(idx, rowHeight);
    updateContentHeight();

    // enable/disable control buttons
    updateButtonsState();
}

void PmMatchListWidget::toggleExpand(bool on)
{
	if (m_tableWidget->rowCount() <= 1)
		on = false;

	PmSpoilerWidget::toggleExpand(on);

    updateButtonsState();
}

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

    updateButtonsState();

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
	int idx = currentIndex();
	if (!isExpanded() || idx < 0) {
		idx = m_tableWidget->rowCount() - 1;
    }
    emit sigMatchConfigInsert((size_t)idx, PmMatchConfig());
    emit sigMatchConfigSelect((size_t)idx);

    if (!isExpanded()) expand();

    QTableWidgetItem* cellItem = m_tableWidget->item(idx, 0);
    if (cellItem)
        m_tableWidget->scrollToItem(cellItem);
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
    //m_tableWidget->item(idx, (int)ColOrder::EnableBox))

    // config label edit
    QString placeholderName = QString("placeholder %1").arg(idx);
    auto labelItem = new QTableWidgetItem(placeholderName);
    labelItem->setTextAlignment(Qt::AlignVCenter);
    labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
    m_tableWidget->setItem(
        idx, (int)ColOrder::ConfigName, labelItem);

    // match actions
    PmReactionDisplay *matchActionsDisplay
        = new PmReactionDisplay("--", parent);
    matchActionsDisplay->setStyleSheet(k_transpBgStyle);
    matchActionsDisplay->setAlignment(Qt::AlignVCenter);
    matchActionsDisplay->installEventFilter(this);
    m_tableWidget->setCellWidget(idx, (int)ColOrder::MatchActions,
        matchActionsDisplay);

    // unmatch actions
    PmReactionDisplay *unmatchActionsDisplay =
        new PmReactionDisplay("--", parent);
    unmatchActionsDisplay->setStyleSheet(k_transpBgStyle);
    unmatchActionsDisplay->setAlignment(Qt::AlignVCenter);
    unmatchActionsDisplay->installEventFilter(this);
    m_tableWidget->setCellWidget(idx, (int)ColOrder::UnmatchActions,
        unmatchActionsDisplay);

    // linger delay
    QSpinBox *lingerDelayBox = new QSpinBox();
    lingerDelayBox->setStyleSheet(k_transpBgStyle);
    lingerDelayBox->setRange(0, 10000);
    lingerDelayBox->setSingleStep(10);
    void (QSpinBox::*sigLingerValueChanged)(int value)
        = &QSpinBox::valueChanged;
    connect(lingerDelayBox, sigLingerValueChanged,
        [this, idx](int val) { lingerDelayChanged(idx, val); });
    lingerDelayBox->setMaximumHeight(lingerDelayBox->sizeHint().height());
    lingerDelayBox->installEventFilter(this);
    PmContainerWidget *lingerContainer = new PmContainerWidget(lingerDelayBox);
    lingerContainer->installEventFilter(this);
    m_tableWidget->setCellWidget(idx, (int)ColOrder::Linger, lingerContainer);

    // cooldown delay
    QSpinBox *cooldownDelayBox = new QSpinBox();
    cooldownDelayBox->setStyleSheet(k_transpBgStyle);
    cooldownDelayBox->setRange(0, 10000);
    cooldownDelayBox->setSingleStep(10);
    void (QSpinBox::*sigcooldownValueChanged)(int value) =
        &QSpinBox::valueChanged;
    connect(cooldownDelayBox, sigcooldownValueChanged,
        [this, idx](int val) { cooldownChanged(idx, val); });
    cooldownDelayBox->setMaximumHeight(cooldownDelayBox->sizeHint().height());
    cooldownDelayBox->installEventFilter(this);
    PmContainerWidget *cooldownContainer
        = new PmContainerWidget(cooldownDelayBox);
    cooldownContainer->installEventFilter(this);
    m_tableWidget->setCellWidget(idx, (int)ColOrder::Cooldown,
				 cooldownContainer);

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

void PmMatchListWidget::updateButtonsState()
{
	bool expanded = isExpanded();
    m_cfgInsertBtn->setVisible(true);
	m_cfgMoveUpBtn->setVisible(expanded);
    m_cfgMoveDownBtn->setVisible(expanded);
	m_cfgRemoveBtn->setVisible(expanded);
    m_buttonSpacer1->changeSize(expanded ? 20 : 0, 0);
	m_buttonSpacer2->changeSize(expanded ? 20 : 0, 0);
    m_buttonsLayout->update();

    int currIdx = currentIndex();
    m_cfgMoveUpBtn->setEnabled(m_core->matchConfigCanMoveUp(currIdx));
    m_cfgMoveDownBtn->setEnabled(m_core->matchConfigCanMoveDown(currIdx));
    m_cfgRemoveBtn->setEnabled(currIdx < m_tableWidget->rowCount()-1);
}

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

void PmMatchListWidget::lingerDelayChanged(int idx, int lingerMs)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    cfg.reaction.lingerMs = lingerMs;
    emit sigMatchConfigChanged(index, cfg);
}

void PmMatchListWidget::cooldownChanged(int idx, int cooldownMs)
{
    size_t index = (size_t)(idx);
    PmMatchConfig cfg = m_core->matchConfig(index);
    cfg.reaction.cooldownMs = cooldownMs;
    emit sigMatchConfigChanged(index, cfg);
}

void PmMatchListWidget::setMinWidth()
{
    int width = m_tableWidget->horizontalHeader()->length() + 20;
    m_tableWidget->setMinimumWidth(width);
}

int PmMatchListWidget::maxContentHeight() const
{
	//return PmSpoilerWidget::contentHeight();

	auto vertHeader = m_tableWidget->verticalHeader();
	auto horizHeader = m_tableWidget->horizontalHeader();

	int count = vertHeader->count();
	//int scrollBarHeight = m_tableWidget->horizontalScrollBar()->height();
	int horizontalHeaderHeight = horizHeader->height();
	int rowTotalHeight = 0;
	for (int i = 0; i < count; ++i) {
		rowTotalHeight += vertHeader->sectionSize(i);
		rowTotalHeight += k_rowPadding;
	}
	return rowTotalHeight + horizontalHeaderHeight;
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
};

QSize PmResultsLabel::sizeHint() const
{
    QSize ret = QLabel::sizeHint();
    ret.setWidth(m_resultTextWidth);
    return ret;
}

//---------------------------------------------------------

PmReactionDisplay::PmReactionDisplay(const QString &text, QWidget *parent)
: QLabel(parent)
, m_fontMetrics(font())
{
    setTextFormat(Qt::RichText);
    setText(text);
    auto m = contentsMargins();
    m_marginsWidth = m.left() + m.right() + margin() * 2 + 10;
}

void PmReactionDisplay::updateReaction(
    const PmReaction &reaction, PmReactionType rtype)
{
	int maxLength = 0;
    const std::vector<PmAction> &actions = (rtype == PmReactionType::Match)
        ? reaction.matchActions : reaction.unmatchActions;
    QString line;
    QString html;
    for (size_t i = 0; i < actions.size(); ++i) {
        const auto &action = actions[i];
        switch (action.m_actionType) {
        case PmActionType::Scene:
		    line = QString("%1%2")
			    .arg(action.m_targetElement.data())
                .arg(action.m_targetDetails.size() ?
                    QString(" [%1]").arg(action.m_targetDetails.data()) : "");
            html += QString("<font color=\"%1\">%2</font>")
				.arg(action.actionColorStr())
                .arg(line);
            break;
        case PmActionType::SceneItem:
        case PmActionType::Filter:
            line = QString("%1 [%2]")
                .arg(action.m_targetElement.data())
                .arg(action.m_actionCode == int(PmToggleCode::Show) ?
                    obs_module_text("show") : obs_module_text("hide"));
            html += QString("<font color=\"%1\">%2</font>")
                .arg(action.actionColorStr())
                .arg(line);
            break;
        case PmActionType::Hotkey:
            // TODO
            break;
        case PmActionType::FrontEndEvent:
            // TODO
            break;
	    default:
    		line = obs_module_text("<not selected>");
		    html += QString("<font color=\"%1\">%2</font>")
			    .arg(action.actionColorStr())
			    .arg(line);
		    break;
        }
	    maxLength = qMax(maxLength, m_fontMetrics.size(0, line).width());
        if (i < actions.size() - 1) {
            html += "<br />";
        }
    }
    setText(html);
    m_textWidth = maxLength;
    m_textHeight = m_fontMetrics.height() * int(actions.size());
    setMinimumSize(m_textWidth + m_marginsWidth, m_textHeight + m_marginsWidth);
    updateGeometry();
}

QSize PmReactionDisplay::sizeHint() const
{
    return QSize(m_textWidth + m_marginsWidth, m_textHeight + m_marginsWidth);
}

PmContainerWidget::PmContainerWidget(QWidget *contained, QWidget *parent)
: QWidget(parent)
, m_contained(contained)
{
	m_contained->setParent(this);
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->addWidget(contained);
    setLayout(m_mainLayout);

    m_mainLayout->setAlignment(Qt::AlignCenter);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
}
