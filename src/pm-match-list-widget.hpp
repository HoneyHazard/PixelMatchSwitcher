#pragma once

#include "pm-spoiler-widget.hpp"
#include "pm-structs.hpp"

#include <QSet>
#include <QLabel>

class PmCore;
class PmReactionWidget;

class QTableWidget;
class QPushButton;
class QComboBox;
class QTableWidgetItem;
class QVBoxLayout;
class QSpacerItem;

/**
 * @brief Shows a list of match configuration entries and allows changing
 *        some of their parameters
  */
class PmMatchListWidget : public PmSpoilerWidget
{
    Q_OBJECT

public:
    PmMatchListWidget(PmCore* core, QWidget* parent);

    int currentIndex() const;

signals:
    void sigMatchConfigChanged(size_t idx, PmMatchConfig cfg);
    
    void sigMatchConfigSelect(size_t matchIndex);
    void sigMatchConfigMoveUp(size_t matchIndex);
    void sigMatchConfigMoveDown(size_t matchIndex);
    void sigMatchConfigInsert(size_t matchIndex, PmMatchConfig cfg);
    void sigMatchConfigRemove(size_t matchIndex);

public slots:
    void toggleExpand(bool on) override;

protected slots:
    // core event handlers
    void onNewMatchResults(size_t idx, PmMatchResults results);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onMatchConfigChanged(size_t idx, PmMatchConfig cfg);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig config);

    // local UI handlers
    void onRowSelected();
    void onItemChanged(QTableWidgetItem* item);
    void onConfigInsertButtonReleased();
    void onConfigRemoveButtonReleased();
    void onConfigMoveUpButtonReleased();
    void onConfigMoveDownButtonReleased();
    void onCellChanged(int row, int col);

protected:
    enum class ColOrder;
    enum class ActionStackOrder;
    static const QStringList k_columnLabels;

    static const QString k_dontSwitchStr;
    static const QString k_sceneItemsLabelStr;
    static const QString k_scenesLabelStr;
    static const QString k_showLabelStr;
    static const QString k_hideLabelStr;

    static const QColor k_dontSwitchColor;
    static const QColor k_scenesLabelColor;
    static const QColor k_scenesColor;
    static const QColor k_sceneItemsLabelColor;
    static const QColor k_sceneItemsColor;

    static const QString k_transpBgStyle;
    static const QString k_semiTranspBgStyle;
    static const int k_rowPadding = 5;

    QPushButton* prepareButton(
        const char *tooltip, const char* icoPath, const char* themeId);
    void updateButtonsState();
    void constructRow(int idx);
    void enableConfigToggled(int idx, bool enable);
    void sceneTransitionSelected(int idx, const QString &transTr);
    void sceneItemActionSelected(int idx, const QString &actionStr);
    
    void lingerDelayChanged(int idx, int lingerMs);
    void cooldownChanged(int idx, int cooldown);

    void setMinWidth();
    bool selectRowAtGlobalPos(QPoint globalPos);
    bool eventFilter(QObject *obj, QEvent *event) override; // for display events

    int maxContentHeight() const override;

    PmCore* m_core;
    QTableWidget *m_tableWidget;

    QPushButton* m_cfgMoveUpBtn;
    QPushButton* m_cfgMoveDownBtn;
    QPushButton* m_cfgInsertBtn;
    QPushButton* m_cfgRemoveBtn;

    QSpacerItem *m_buttonSpacer1;
    QSpacerItem *m_buttonSpacer2;
    QHBoxLayout *m_buttonsLayout;

    int m_prevMatchIndex = 0;
};

class PmResultsLabel : public QLabel
{
    Q_OBJECT

public:
    PmResultsLabel(const QString &text, QWidget *parentWidget);
    QSize sizeHint() const override;

protected:
    int m_resultTextWidth = 0;
};

class PmContainerWidget : public QWidget
{
    Q_OBJECT

public:
    PmContainerWidget(QWidget *contained, QWidget *parent = nullptr);
	QWidget *containedWidget() const { return m_contained; }
    QVBoxLayout *mainLayout() const { return m_mainLayout; }

protected:
    void mouseReleaseEvent(QMouseEvent *) override
        { m_contained->setFocus(); }

    QWidget *m_contained;
    QVBoxLayout *m_mainLayout;
};

class PmReactionDisplay : public QLabel
{
    Q_OBJECT

public:
    PmReactionDisplay(const QString &text, QWidget *parent);
    void updateReaction(const PmReaction &reaction, PmReactionType rtype);
    QSize sizeHint() const override;

protected:
    void enterEvent(QEvent *event) override;
	void leaveEvent(QEvent *event) override;

protected:
	void updateContents(const QString &html, int textWidthMax, int textRows);

    int m_textWidth = 0;
	int m_textHeight = 0;
	int m_marginsWidth = 0;
    QFontMetrics m_fontMetrics;
    bool m_hasActions = false;
};
