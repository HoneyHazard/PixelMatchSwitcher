#pragma once

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"
#include "pm-spoiler-widget.hpp"

#include <vector>
#include <QMultiHash>

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

/**
 * @brief Shows and configures individual action of a match/unmatmch reaction
 */
// TODO: serious refactor
class PmActionEntryWidget : public QWidget
{
    Q_OBJECT

public:
    PmActionEntryWidget(PmCore *core, size_t actionIndex, QWidget *parent);

    void actionToUi(
        size_t actionIndex, PmAction action, const std::string &cfgLabel);

    void installEventFilterAll(QObject *obj);

signals:
    void sigActionChanged(size_t actionIndex, PmAction action);

public slots:
    void onScenesChanged();

protected slots:
    void onUiChanged();

    // ui customization
    void onHotkeySelectionChanged();
    void onFileBrowseReleased();
    void onFileStringsChanged();
    void onShowFileMarkersHelp();
    void onShowFilePreview();
    void onShowTimeFormatHelp();

protected:
    static const QString k_defaultTransitionStr;
    static const int k_keyRole;
    static const int k_modifierRole;
    static const int k_keyHintRole;
    static const char *k_timeFormatHelpUrl;
    static const char *k_fileMarkersHelpHtml;

    struct HotkeyData {
        obs_hotkey_t *hkey;
        obs_key_combination_t keyCombo;
    };
    typedef QList<HotkeyData> HotkeysList;
    typedef QMultiHash<std::string, HotkeyData> HotkeysGroup;

    void insertHotkeysList(
        int &idx, const QString &info, HotkeysList list);
    void insertHotkeysGroup(
        int &idx, const QString &category, const HotkeysGroup &group);
    void insertFrontendEntries(
        int &idx, const QString &category,
        PmFrontEndAction start, PmFrontEndAction end); 

    void prepareSelections();
    void updateScenes();
    void updateAudioSources();
    void updateTransitons();
    void updateHotkeys();
    void updateFrontendActions();
    void updateUiStyle(const PmAction &action);
    void selectDetailsWidget(QWidget *widget);
    void showFileActionsUi(bool on);

    QComboBox *m_targetCombo;

    QComboBox *m_transitionsCombo;
    QComboBox *m_toggleSourceCombo;
    QComboBox *m_toggleMuteCombo;

    QLabel *m_hotkeyDetailsLabel;

    QComboBox *m_fileActionCombo;
    QPushButton *m_fileMarkersHelpButton;
    QLineEdit *m_filenameEdit;
    QPushButton *m_fileBrowseButton;
    QLineEdit *m_fileTextEdit;
    QLabel *m_fileTimeFormatLabel;
    QLineEdit *m_fileTimeFormatEdit;
    QPushButton *m_fileStringsPreviewButton;
    QPushButton *m_fileTimeFormatHelpButton;
    QWidget *m_fileActionsWidget;

    QStackedWidget *m_detailsStack;

    PmCore *m_core;
    size_t m_actionIndex;
    PmActionType m_actionType = PmActionType::None;
    QString m_filePreviewStr;
};

//----------------------------------------------------

/**
 * @briefs Shows and configures a match/unmatch reaction, which is a combination
 *         of actions
 */
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
    void reactionToUi(const PmReaction &reaction, const std::string &cfgLabel);
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
