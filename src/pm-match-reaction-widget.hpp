#pragma once

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"
#include "pm-spoiler-widget.hpp"

#include <vector>

class PmCore;
class PmAddActionMenu;

class QLineEdit;
class QPushButton;
class QToolButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QVBoxLayout;
class QScrollArea;
class QWidgetAction;
class QLabel;
class QEvent;

class PmActionEntryWidget : public QWidget
{
    Q_OBJECT

public:
    PmActionEntryWidget(PmCore *core, size_t actionIndex, QWidget *parent);

    void updateSizeHints(QList<QSize> &columnSizes);

    void updateAction(size_t actionIndex, PmAction action);
    void updateHotkeys(); // TODO
    void updateFrontendEvents(); // TODO

    void installEventFilterAll(QObject *obj);

signals:
    void sigActionChanged(size_t actionIndex, PmAction action);

public slots:
    void onScenesChanged();

protected slots:
    void onActionTypeSelectionChanged();
    void onUiChanged();
    void onHotkeySelectionChanged();

protected:
    static const QString k_defaultTransitionStr;
	static const int k_hotkeyInfoRole;

    struct HotkeyData {
	    obs_hotkey_t *hkey;
	    std::string comboStr;
    };

	typedef QList<HotkeyData> HotkeysList;
	typedef QMultiHash<std::string, HotkeyData> HotkeysGroup;

    void insertHotkeysList(
        int &idx, const QString &info, HotkeysList list);
    void insertHotkeysGroup(
        int &idx, const QString &category, const HotkeysGroup &group);

    void prepareSelections();
    void updateScenes();
    void updateTransitons();
    void updateUiStyle(const PmAction &action);

    QComboBox *m_targetCombo;

    QComboBox *m_transitionsCombo;
    QComboBox *m_toggleCombo;
    QLabel *m_detailsLabel;
    QStackedWidget *m_detailsStack;



    PmCore *m_core;
    size_t m_actionIndex;
    PmActionType m_actionType = PmActionType::None;
};

//----------------------------------------------------

class PmMatchReactionWidget : public PmSpoilerWidget
{
    Q_OBJECT

public:
    PmMatchReactionWidget(
        PmCore *core, PmAddActionMenu *addActionMenu,
        PmReactionTarget reactionTarget, PmReactionType reactionType,
        QWidget *parent);

signals:
    void sigMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void sigNoMatchReactionChanged(PmReaction reaction);

public slots:
    void toggleExpand(bool on) override;

protected slots:
    // core events
    void onMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig cfg);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onNoMatchReactionChanged(PmReaction reaction);
    void onActionChanged(size_t actionIndex, PmAction action);

    // local UI events
    void onInsertReleased();
    void onRemoveReleased();
    void updateButtonsState();
    void updateTitle();

protected:
    PmReaction pullReaction() const;
    void pushReaction(const PmReaction &reaction);
    void reactionToUi(const PmReaction &reaction);
    int maxContentHeight() const override;
    bool eventFilter(QObject *obj, QEvent *event) override; // for display events


    QListWidget *m_actionListWidget;
    QPushButton *m_insertActionButton;
    QPushButton *m_removeActionButton;

    PmReactionTarget m_reactionTarget;
    PmReactionType m_reactionType;
    size_t m_matchIndex = (size_t)-1;
    size_t m_multiConfigSz = 0;
    size_t m_lastActionCount = (size_t)-1;

    PmCore *m_core;
    PmAddActionMenu *m_addActionMenu;
};

