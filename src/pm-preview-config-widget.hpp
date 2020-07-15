#pragma once

#include <QGroupBox>

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

class PmCore;

class QComboBox;
class QStackedWidget;
class QButtonGroup;

/**
 * @brief Widget for configuring preview settings
 */
class PmPreviewConfigWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmPreviewConfigWidget(PmCore* core, QWidget* parent);

signals:
    void sigPreviewConfigChanged(PmPreviewConfig cfg);

protected slots:
    // reaction to core events

    void onPreviewConfigChanged(PmPreviewConfig cfg);
    void onSelectMatchIndex(size_t matchindex, PmMatchConfig cfg);
    //void onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg);
    void onImgSuccess(size_t matchIndex);
    void onImgFailed(size_t matchIndex);
    void onNewActiveFilter(PmFilterRef ref);
    void onRunningEnabledChanged(bool enable);

    // parse UI state into config
    void onConfigUiChanged();

protected:
    void enableRegionViews(bool enable);

    QButtonGroup* m_previewModeButtons;
    QStackedWidget* m_previewScaleStack;
    QComboBox* m_videoScaleCombo;
    QComboBox* m_regionScaleCombo;
    QComboBox* m_matchImgScaleCombo;

    PmCore* m_core;
    size_t m_matchIndex = 0;
};