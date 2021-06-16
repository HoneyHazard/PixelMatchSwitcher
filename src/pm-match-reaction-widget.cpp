#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"
#include "pm-core.hpp"
#include "pm-add-action-menu.hpp"

#include <obs-module.h>
#include <util/dstr.hpp>

#include <QComboBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QStandardItemModel>
#include <QMouseEvent>
#include <QMultiHash>
#include <QLabel>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default transition>");
const int PmActionEntryWidget::k_keyRole = Qt::UserRole + 1;
const int PmActionEntryWidget::k_modifierRole = Qt::UserRole + 2;
const int PmActionEntryWidget::k_keyHintRole = Qt::UserRole + 3;

PmActionEntryWidget::PmActionEntryWidget(
    PmCore* core, size_t actionIndex, QWidget *parent)
: QWidget(parent)
, m_core(core)
, m_actionIndex(actionIndex)
{
    // for all types of targets
    m_targetCombo = new QComboBox(this);

    // transitions for scenes
    m_transitionsCombo = new QComboBox(this);

    // toggle scene items and filters on and off
    m_toggleCombo = new QComboBox(this);
    m_toggleCombo->addItem(
        obs_module_text("Show"), (unsigned int)(PmToggleCode::Show));
    m_toggleCombo->addItem(
        obs_module_text("Hide"), (unsigned int)PmToggleCode::Hide);

    // hotkeys
    m_hotkeyPressReleaseCombo = new QComboBox(this);
    m_hotkeyPressReleaseCombo->addItem(
        obs_module_text("Press + Release"), size_t(PmHotkeyActionCode::Both));
    m_hotkeyPressReleaseCombo->addItem(
        obs_module_text("Press"), size_t(PmHotkeyActionCode::Press));
    m_hotkeyPressReleaseCombo->addItem(
        obs_module_text("Release"), size_t(PmHotkeyActionCode::Release));
    
    m_hotkeyDetailsLabel = new QLabel(this);
    m_hotkeyDetailsLabel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    QHBoxLayout *hotkeyLayout = new QHBoxLayout;
    hotkeyLayout->addWidget(m_hotkeyPressReleaseCombo);
    hotkeyLayout->addSpacerItem(new QSpacerItem(10, 0));
    hotkeyLayout->addWidget(m_hotkeyDetailsLabel);
    hotkeyLayout->setContentsMargins(0, 0, 0, 0);
    m_hotkeyWidget = new QWidget(this);
    m_hotkeyWidget->setLayout(hotkeyLayout);

    // selectively shows and selects details for different types of targets
    m_detailsStack = new QStackedWidget(this);
    m_detailsStack->addWidget(m_transitionsCombo);
    m_detailsStack->addWidget(m_toggleCombo);
    m_detailsStack->addWidget(m_hotkeyWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_targetCombo);
    mainLayout->addWidget(m_detailsStack);
    setLayout(mainLayout);

    // local UI connections
    connect(m_targetCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_transitionsCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_toggleCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_toggleCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onHotkeySelectionChanged);

    onScenesChanged();
}

void PmActionEntryWidget::updateScenes()
{
	QBrush sceneBrush
        = PmAction::dimmedColor(PmActionType::Scene, m_actionType);
    QBrush siBrush = PmAction::dimmedColor(
        PmActionType::SceneItem, m_actionType);
	QBrush fiBrush = PmAction::dimmedColor(
        PmActionType::Filter, m_actionType);

    QList<std::string> scenes = m_core->sceneNames();
    auto model = dynamic_cast<QStandardItemModel *>(m_targetCombo->model());
    int idx;

    PmActionType aType = m_actionType;
    m_targetCombo->blockSignals(true);
    m_targetCombo->clear();
    QString selStr = QString("<%1 %2>")
		.arg(obs_module_text("select"))
        .arg(PmAction::actionStr(aType));
    m_targetCombo->addItem(selStr, "");
    model->item(0)->setEnabled(false);
    for (const std::string &scene : scenes) {
	    m_targetCombo->addItem(scene.data(), scene.data());
	    idx = m_targetCombo->count() - 1; 
	    m_targetCombo->setItemData(idx, sceneBrush, Qt::ForegroundRole);
	    if (aType != PmActionType::Scene) {
		    QList<std::string> siNames = m_core->sceneItemNames(scene);
		    if (siNames.empty()) {
			    m_targetCombo->removeItem(idx);
			    continue;
            }
		    model->item(idx)->setEnabled(false);
		    for (const std::string &siName : siNames) {
			    m_targetCombo->addItem(
                    QString("  ") + siName.data(), siName.data());
			    idx = m_targetCombo->count() - 1;
                m_targetCombo->setItemData(idx, siBrush, Qt::ForegroundRole);
			    if (aType != PmActionType::SceneItem) {
                    QList<std::string> fiNames = m_core->filterNames(siName);
				    if (fiNames.empty()) {
                        m_targetCombo->removeItem(idx);
					    continue;
                    }
				    model->item(idx)->setEnabled(false);
                    for (const std::string& fiName : fiNames) {
                        m_targetCombo->addItem(
                            QString("    ") + fiName.data(), fiName.data());
			            idx = m_targetCombo->count() - 1;
                        m_targetCombo->setItemData(
                            idx, fiBrush, Qt::ForegroundRole);
                    }
                }
            }
	    }
    }
    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::updateTransitons()
{
    QList<std::string> transitions = m_core->availableTransitions();

    m_transitionsCombo->blockSignals(true);
    m_transitionsCombo->clear();
    m_transitionsCombo->addItem(k_defaultTransitionStr, QVariant(QString()));
    for (const std::string &t : transitions) {
        m_transitionsCombo->addItem(t.data(), t.data());
    }
    m_transitionsCombo->blockSignals(false);
}

void PmActionEntryWidget::updateSizeHints(QList<QSize> &columnSizes)
{
    QList<QSize> sizes = {
        //m_actionTypeCombo->sizeHint(),
        m_targetCombo->sizeHint(),
        m_detailsStack->sizeHint()
    };
    while (columnSizes.size() < 2) {
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
    if (m_actionIndex != actionIndex) return;

    PmActionType prevType = m_actionType;
    if (prevType != action.actionType) {
        m_actionType = action.actionType;
        prepareSelections();
    }

    switch (action.actionType) {
    case PmActionType::None:
        m_targetCombo->setVisible(false);
        m_detailsStack->setVisible(false);
        break;
    case PmActionType::Scene:
        m_targetCombo->setVisible(true);
        m_targetCombo->blockSignals(true);
        m_targetCombo->setCurrentText(action.targetElement.data());
        m_targetCombo->blockSignals(false);

        m_transitionsCombo->blockSignals(true);
        m_transitionsCombo->setCurrentText(action.targetDetails.data());
        m_transitionsCombo->blockSignals(false);

        m_detailsStack->setVisible(true);
	    m_detailsStack->setCurrentWidget(m_transitionsCombo);
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
         m_targetCombo->setVisible(true);
         m_targetCombo->blockSignals(true);
         m_targetCombo->setCurrentText(action.targetElement.data());
         m_targetCombo->blockSignals(false);

         m_toggleCombo->blockSignals(true);
         m_toggleCombo->setCurrentIndex(
            m_toggleCombo->findData(action.actionCode));
         m_toggleCombo->blockSignals(false);

         m_detailsStack->setVisible(true);
	     m_detailsStack->setCurrentWidget(m_toggleCombo);
         break;
#if 0
    case PmActionType::Filter:
        //m_targetCombo->setVisible(true);
        m_targetCombo->blockSignals(true);
        m_targetCombo->setCurrentText(action.m_targetElement.data());
        m_targetCombo->blockSignals(false);

        m_toggleCombo->blockSignals(true);
        m_toggleCombo->setCurrentIndex(
            m_toggleCombo->findData(action.m_actionCode));
        m_toggleCombo->blockSignals(false);
        break;
#endif
    case PmActionType::Hotkey:
	    m_targetCombo->setVisible(true);
	    m_targetCombo->blockSignals(true);
	    m_hotkeyPressReleaseCombo->blockSignals(true);
	    if (action.isSet()) {
		    DStr dstr;
		    obs_key_combination_to_str(action.keyCombo, dstr);
		    int targetIdx = m_targetCombo->findData((const char*)dstr);
			m_targetCombo->setCurrentIndex(targetIdx);
		    int pressReleaseIdx
                = m_hotkeyPressReleaseCombo->findData((size_t)action.actionCode);
			m_hotkeyPressReleaseCombo->setCurrentIndex(pressReleaseIdx);
	    } else {
		    m_targetCombo->setCurrentIndex(0);
		    m_hotkeyPressReleaseCombo->setCurrentIndex(0);
        }
        m_targetCombo->blockSignals(false);
	    m_hotkeyPressReleaseCombo->blockSignals(false);

        m_detailsStack->setVisible(true);
        m_detailsStack->setCurrentWidget(m_hotkeyWidget);
	    onHotkeySelectionChanged();
        break;
    }

    updateUiStyle(action);
}

void PmActionEntryWidget::insertHotkeysList(
    int &idx, const QString &info, HotkeysList list)
{
	if (list.isEmpty()) return;

    #if 0
	std::stable_sort(begin(list), end(list),
		[&](const HotkeyData &a, const HotkeyData &b) {
		const auto &nameA = std::get<2>(a);
		const auto &nameB = std::get<2>(b);
		return nameA < nameB;
	});
    #endif

    auto model = (QStandardItemModel *)m_targetCombo->model();
	QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::Hotkey);
    QString header = QString("---- %1 ----").arg(info);
    m_targetCombo->addItem(header);
	model->item(idx)->setEnabled(false);    m_targetCombo->setItemData(idx, dimmedBrush, Qt::ForegroundRole);
    idx++;

    QBrush selBrush = PmAction::actionColor(PmActionType::Hotkey);
    for (const auto &hkeyData : list) {
	    const char *descr = obs_hotkey_get_description(hkeyData.hkey);
	    DStr comboStr;
	    obs_key_combination_to_str(hkeyData.keyCombo, comboStr);

	    QString targetStr = QString("%1 (%2)")
            .arg(descr).arg((const char*)comboStr);

        m_targetCombo->addItem(targetStr, (const char*)comboStr);
	    m_targetCombo->setItemData(idx, selBrush, Qt::ForegroundRole);
	    m_targetCombo->setItemData(
            idx, (int)hkeyData.keyCombo.key, k_keyRole);
        m_targetCombo->setItemData(
            idx, (uint32_t)hkeyData.keyCombo.modifiers, k_modifierRole);
	    m_targetCombo->setItemData(idx, info, k_keyHintRole);
	    idx++;
    }
}

void PmActionEntryWidget::insertHotkeysGroup(
    int &idx, const QString &category, const HotkeysGroup &group)
{
	//auto model = (QStandardItemModel *)m_targetCombo->model();
	//QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::Hotkey);

	for (const auto &key : group.keys()) {
		QString info = QString("[%1] %2").arg(category).arg(key.data());
		insertHotkeysList(idx, info, group.values(key));
    }
}

void PmActionEntryWidget::updateHotkeys()
{
    HotkeysList allKeys;
    HotkeysList frontendKeys;
    HotkeysGroup outputsKeys;
    HotkeysGroup encodersKeys;
    HotkeysGroup servicesKeys;
    HotkeysGroup scenesKeys;
    HotkeysGroup sourcesKeys;

    #if 0
    obs_enum_hotkeys(
        [](void *data, obs_hotkey_id id, obs_hotkey_t *key) -> bool
        {
		    HotkeysList *hotkeys = (HotkeysList *)data;
		    hotkeys->push_back(
                std::make_tuple(key, id, obs_hotkey_get_description(key)));
		    return true;
        },
        (void*)&allKeys);
    #endif

    obs_enum_hotkey_bindings(
	    [](void *data, size_t idx, obs_hotkey_binding_t *binding) -> bool {
		    HotkeysList *hotkeys = (HotkeysList *)data;
		    //obs_hotkey_id id =
			//    obs_hotkey_binding_get_hotkey_id(binding);
		    obs_hotkey_t* key = obs_hotkey_binding_get_hotkey(binding);
		    obs_key_combination_t combo =
			    obs_hotkey_binding_get_key_combination(binding);
            HotkeyData hkeyData = { key, combo };
		    hotkeys->push_back(hkeyData);
            UNUSED_PARAMETER(idx);
            return true;
	    },
	    (void *)&allKeys);


    for (const auto &hkeyData : allKeys) {
	    auto hkey = hkeyData.hkey;
	    obs_hotkey_registerer_type rtype =
		    obs_hotkey_get_registerer_type(hkey);
	    void *registerer = obs_hotkey_get_registerer(hkey);
	    switch (rtype) {
	    case OBS_HOTKEY_REGISTERER_FRONTEND:
		    frontendKeys.push_back(hkeyData);
		    break;

	    case OBS_HOTKEY_REGISTERER_SOURCE: {
		    auto weakSource =
			    static_cast<obs_weak_source_t *>(registerer);
		    auto source = OBSGetStrongRef(weakSource);
		    if (source) {
			    auto name = obs_source_get_name(source);
			    if (obs_scene_t *scene =
					obs_scene_from_source(source)) {
				    scenesKeys.insert(name, hkeyData);
			    } else {
				    sourcesKeys.insert(name, hkeyData);
			    }
		    }
	    } break;

	    case OBS_HOTKEY_REGISTERER_OUTPUT: {
		    auto weakOutput =
			    static_cast<obs_weak_output_t *>(registerer);
		    auto output = OBSGetStrongRef(weakOutput);
		    if (output) {
			    auto name = obs_output_get_name(output);
			    outputsKeys.insert(name, hkeyData);
		    }
	    } break;

	    case OBS_HOTKEY_REGISTERER_ENCODER: {
		    auto weakEncoder =
			    static_cast<obs_weak_encoder_t *>(registerer);
		    auto encoder = OBSGetStrongRef(weakEncoder);
		    if (encoder) {
			    auto name = obs_encoder_get_name(encoder);
			    encodersKeys.insert(name, hkeyData);
		    }
	    } break;

	    case OBS_HOTKEY_REGISTERER_SERVICE: {
		    auto weakService =
			    static_cast<obs_weak_service_t *>(registerer);
		    auto service = OBSGetStrongRef(weakService);
		    if (service) {
			    auto name = obs_service_get_name(service);
			    servicesKeys.insert(name, hkeyData);
		    }
	    } break;
	    }
    }

    auto model = (QStandardItemModel *)m_targetCombo->model();
    QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::Hotkey);

    m_targetCombo->blockSignals(true);
    m_targetCombo->clear();
    int idx = 0;
    m_targetCombo->addItem(obs_module_text("<select hotkey>"), (unsigned int)-1);
    model->item(idx)->setEnabled(false);
    m_targetCombo->setItemData(idx, dimmedBrush, Qt::ForegroundRole);
    idx++;

    insertHotkeysList(idx, obs_module_text("[frontend]"), frontendKeys);
    insertHotkeysGroup(idx, obs_module_text("output"), outputsKeys);
    insertHotkeysGroup(idx, obs_module_text("encoder"), encodersKeys);
    insertHotkeysGroup(idx, obs_module_text("service"), servicesKeys);
    insertHotkeysGroup(idx, obs_module_text("scene"), scenesKeys);
    insertHotkeysGroup(idx, obs_module_text("source"), sourcesKeys);

    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::updateFrontendEvents()
{

}

void PmActionEntryWidget::installEventFilterAll(QObject *obj)
{
	for (QWidget *w : QObject::findChildren<QWidget *>()) {
		w->installEventFilter(obj);
    }
}

void PmActionEntryWidget::prepareSelections()
{
    switch (m_actionType) {
    case PmActionType::Scene:
        updateScenes();
        updateTransitons();
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        updateScenes();
        break;
    case PmActionType::Hotkey:
	    updateHotkeys();
        break;
    default:
	    break;
    }
}

void PmActionEntryWidget::onActionTypeSelectionChanged()
{
    prepareSelections();
    onUiChanged();
}

void PmActionEntryWidget::onUiChanged()
{
    PmAction action;
	action.actionType = m_actionType;

    switch (PmActionType(m_actionType)) {
    case PmActionType::None:
        break;
    case PmActionType::Scene:
        action.targetElement
            = m_targetCombo->currentData().toString().toUtf8().data();
        action.targetDetails
            = m_transitionsCombo->currentData().toString().toUtf8().data();
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        action.targetElement
            = m_targetCombo->currentData().toString().toUtf8().data();
	    action.actionCode = (size_t)m_toggleCombo->currentData().toUInt();
        break;
    case PmActionType::Hotkey:
	    action.keyCombo.key
            = (obs_key_t) m_targetCombo->currentData(k_keyRole).toInt();
	    action.keyCombo.modifiers
            = (uint32_t) m_targetCombo->currentData(k_modifierRole).toInt();
	    action.actionCode
            = (size_t) m_hotkeyPressReleaseCombo->currentData().toUInt();
	    break;
    }

    emit sigActionChanged(m_actionIndex, action);

    updateUiStyle(action);
}

void PmActionEntryWidget::onHotkeySelectionChanged()
{
	if (m_actionType == PmActionType::Hotkey) {
		QString info = m_targetCombo->currentData(k_keyHintRole).toString();
		m_hotkeyDetailsLabel->setText(info);
    }
}

void PmActionEntryWidget::onScenesChanged()
{
	prepareSelections();
}

void PmActionEntryWidget::updateUiStyle(const PmAction &action)
{
	QString colorStyle = QString("color: %1").arg(action.actionColorStr());

    for (QWidget *w : findChildren<QWidget *>()) {
		if (w != m_hotkeyDetailsLabel)
            w->setStyleSheet(colorStyle);
    }

    m_detailsStack->setStyleSheet(
	    "QStackedWidget { background-color: rgba(0, 0, 0, 0) }");
    //m_hotkeyWidget->setStyleSheet(
	//    "QWidget { background-color: rgba(0, 0, 0, 0) }");
}

//----------------------------------------------------

PmMatchReactionWidget::PmMatchReactionWidget(
    PmCore *core, PmAddActionMenu *addActionMenu,
    PmReactionTarget reactionTarget, PmReactionType reactionType,
    QWidget *parent)
: PmSpoilerWidget(parent)
, m_core(core)
, m_addActionMenu(addActionMenu)
, m_reactionTarget(reactionTarget)
, m_reactionType(reactionType)
{
	QIcon insertIcon;
	insertIcon.addFile(":/res/images/add.png", QSize(), QIcon::Normal,
			   QIcon::Off);
	m_insertActionButton = new QPushButton(this);
	m_insertActionButton->setIcon(insertIcon);
	m_insertActionButton->setIconSize(QSize(16, 16));
	m_insertActionButton->setMaximumSize(22, 22);
	m_insertActionButton->setFlat(true);
	m_insertActionButton->setProperty("themeID", QVariant("addIconSmall"));
	m_insertActionButton->setToolTip(
		obs_module_text("Insert New Action"));
	m_insertActionButton->setFocusPolicy(Qt::NoFocus);
	connect(m_insertActionButton, &QPushButton::released,
            this, &PmMatchReactionWidget::onInsertReleased);

    QIcon removeIcon;
	removeIcon.addFile(
        ":/res/images/list_remove.png", QSize(), QIcon::Normal, QIcon::Off);
    m_removeActionButton = new QPushButton(this);
	m_removeActionButton->setIcon(removeIcon);
    m_removeActionButton->setIconSize(QSize(16, 16));
	m_removeActionButton->setMaximumSize(22, 22);
    m_removeActionButton->setFlat(true);
    m_removeActionButton->setProperty("themeID", QVariant("removeIconSmall"));
    m_removeActionButton->setToolTip(obs_module_text("Remove Action"));
    m_removeActionButton->setFocusPolicy(Qt::NoFocus);
    connect(m_removeActionButton, &QPushButton::released, this,
	        &PmMatchReactionWidget::onRemoveReleased);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_insertActionButton);
    buttonLayout->addWidget(m_removeActionButton);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    setTopRightLayout(buttonLayout);

    m_actionListWidget = new QListWidget(this);
    m_actionListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_actionListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_actionListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_actionListWidget, &QListWidget::currentRowChanged,
            this, &PmMatchReactionWidget::updateButtonsState);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_actionListWidget);
    setContentLayout(mainLayout);
    m_contentArea->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Expanding);

    // entry vs no-match specific
    const auto qc = Qt::QueuedConnection;

    if (reactionTarget == PmReactionTarget::Entry) {
        connect(m_core, &PmCore::sigMatchConfigSelect,
                this, &PmMatchReactionWidget::onMatchConfigSelect, qc);
        connect(m_core, &PmCore::sigMatchConfigChanged,
                this, &PmMatchReactionWidget::onMatchConfigChanged, qc);
        connect(m_core, &PmCore::sigMultiMatchConfigSizeChanged,
                this, &PmMatchReactionWidget::onMultiMatchConfigSizeChanged, qc);

        connect(this, &PmMatchReactionWidget::sigMatchConfigChanged,
                m_core, &PmCore::onMatchConfigChanged, qc);

        onMultiMatchConfigSizeChanged(m_core->multiMatchConfigSize());
        size_t idx = m_core->selectedConfigIndex();
        onMatchConfigSelect(idx, m_core->matchConfig(idx));
    } else { // Anything
        connect(m_core, &PmCore::sigNoMatchReactionChanged,
                this, &PmMatchReactionWidget::onNoMatchReactionChanged, qc);
        connect(this, &PmMatchReactionWidget::sigNoMatchReactionChanged,
                m_core, &PmCore::onNoMatchReactionChanged, qc);

        onNoMatchReactionChanged(m_core->noMatchReaction());
    }
}

void PmMatchReactionWidget::onMatchConfigChanged(
    size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    const PmReaction &reaction = cfg.reaction;
    reactionToUi(reaction);
}

void PmMatchReactionWidget::onNoMatchReactionChanged(PmReaction reaction)
{
    reactionToUi(reaction);
}

PmReaction PmMatchReactionWidget::pullReaction() const
{
    return m_reactionTarget == PmReactionTarget::Entry
        ? m_core->reaction(m_matchIndex) : m_core->noMatchReaction();
}

void PmMatchReactionWidget::pushReaction(const PmReaction &reaction)
{
    if (m_reactionTarget == PmReactionTarget::Entry) {
        PmMatchConfig cfg = m_core->matchConfig(m_matchIndex);
        cfg.reaction = reaction;
        emit sigMatchConfigChanged(m_matchIndex, cfg);
    } else {
        emit sigNoMatchReactionChanged(reaction);
    }
}

void PmMatchReactionWidget::reactionToUi(const PmReaction &reaction)
{
    const auto &actionList = (m_reactionType == PmReactionType::Match) ?
        reaction.matchActions : reaction.unmatchActions;

    size_t listSz = actionList.size();
    for (size_t i = 0; i < listSz; ++i) {
        PmActionEntryWidget *entryWidget = nullptr;
        if (i >= m_actionListWidget->count()) {
            const auto qc = Qt::QueuedConnection;
            entryWidget = new PmActionEntryWidget(m_core, i, this);
            entryWidget->installEventFilterAll(this);
            connect(m_core, &PmCore::sigScenesChanged,
                    entryWidget, &PmActionEntryWidget::onScenesChanged, qc);
            connect(entryWidget, &PmActionEntryWidget::sigActionChanged,
                    this, &PmMatchReactionWidget::onActionChanged, qc);

            QListWidgetItem *item = new QListWidgetItem(m_actionListWidget);
            m_actionListWidget->addItem(item);
            item->setSizeHint(entryWidget->sizeHint());
            m_actionListWidget->setItemWidget(item, entryWidget);
        } else {
            QListWidgetItem* item = m_actionListWidget->item(int(i));
            entryWidget
                = (PmActionEntryWidget*) m_actionListWidget->itemWidget(item);
        }
        entryWidget->updateAction(i, actionList[i]);
    }
    while (m_actionListWidget->count() > listSz) {
        QListWidgetItem *item = m_actionListWidget->takeItem(
            m_actionListWidget->count() - 1);
        delete item;
    }

    int listMinHeight = 10;
    if (listSz > 0) {
	    int idx = m_actionListWidget->currentRow();
	    if (idx < 0) idx = 0;
	    listMinHeight = m_actionListWidget->sizeHintForRow(0);
    } 
    m_actionListWidget->setMinimumHeight(listMinHeight);

    bool expand = (listSz > 0);
    toggleExpand(expand); // calls updateButtonsState() and updateContentHeight()
    updateTitle();

    // will ensure the last item is visible and selected after being added
    if (m_lastActionCount != listSz
     && listSz > 0
     && !actionList[listSz-1].isSet()) {
	    auto lastItem = m_actionListWidget->item((int)listSz - 1);
	    m_actionListWidget->scrollToItem(lastItem);
	    lastItem->setSelected(true);
    }
    m_lastActionCount = listSz;
}

int PmMatchReactionWidget::maxContentHeight() const
{
	int ret = 0;
	int count = m_actionListWidget->count();
	const int extraPadding = 10; // TODO: figure out where it's coming from

	for (int i = 0; i < count; i++) {
		ret += m_actionListWidget->sizeHintForRow(i);
    }
	return ret + extraPadding;
}

bool PmMatchReactionWidget::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = (QMouseEvent *)event;
		auto listPos = m_actionListWidget->mapFromGlobal(
            mouseEvent->globalPos());
		auto item = m_actionListWidget->itemAt(listPos);
		if (item)
			item->setSelected(true);
	}
	return QObject::eventFilter(obj, event);
}

void PmMatchReactionWidget::updateButtonsState()
{
	bool available = true;
	if (m_reactionTarget == PmReactionTarget::Entry) {
		available = m_matchIndex < m_multiConfigSz;
    }
    m_insertActionButton->setVisible(available);
    m_removeActionButton->setVisible(available && isExpanded());

    int actionIdx = m_actionListWidget->currentRow();
    m_removeActionButton->setEnabled(actionIdx >= 0);
}

void PmMatchReactionWidget::updateTitle()
{
	QString str;
	if (m_reactionTarget == PmReactionTarget::Global) {
		str = (m_reactionType == PmReactionType::Match)
		    ? obs_module_text("Anything Matched Actions")
		    : obs_module_text("Nothing Matched Actions");
	} else {
		str = QString(obs_module_text("%1 Actions #%2: %3"))
			.arg((m_reactionType == PmReactionType::Match)
				     ? obs_module_text("Match")
				     : obs_module_text("Unmatch"))
			.arg(m_matchIndex + 1)
			.arg(m_core->matchConfigLabel(m_matchIndex).data());
	}

    if (m_actionListWidget->count() > 0) {
		str += QString(" [%1]").arg(m_actionListWidget->count());
    }
    setTitle(str);
}

void PmMatchReactionWidget::toggleExpand(bool on)
{
	if (m_actionListWidget->count() == 0)
		on = false;

	PmSpoilerWidget::toggleExpand(on);

    updateButtonsState();
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

void PmMatchReactionWidget::onActionChanged(size_t actionIndex, PmAction action)
{
    PmReaction reaction = pullReaction();

    if (m_reactionType == PmReactionType::Match) {
        reaction.matchActions[actionIndex] = action;
    } else {
        reaction.unmatchActions[actionIndex] = action;
    }

    pushReaction(reaction);
}

void PmMatchReactionWidget::onInsertReleased()
{
	m_addActionMenu->setTypeAndTarget(m_reactionTarget, m_reactionType);
	m_addActionMenu->setMatchIndex(m_matchIndex);
	m_addActionMenu->popup(QCursor::pos());
}

void PmMatchReactionWidget::onRemoveReleased()
{
    int idx = 0;
    if (m_actionListWidget->count() > 0) {
        idx = (size_t)m_actionListWidget->currentRow();
        if (idx < 0)
            return;
    } else {                       
        return;
    }

    PmReaction reaction = pullReaction();
    if (m_reactionType == PmReactionType::Match) {
        reaction.matchActions.erase(reaction.matchActions.begin() + idx);
    } else { // Unmatch
        reaction.unmatchActions.erase(reaction.unmatchActions.begin() + idx);
    }
    pushReaction(reaction);
}

