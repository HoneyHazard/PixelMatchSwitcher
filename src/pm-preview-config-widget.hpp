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
    void onMatchConfigSelect(size_t matchindex, PmMatchConfig cfg);
    void onMatchImageLoadSuccess(size_t matchIndex);
    void onMatchImageLoadFailed(size_t matchIndex);
    void onActiveFilterChanged(PmFilterRef ref);
    void onRunningEnabledChanged(bool enable);

    // parse UI state into config
    void onPreviewModeRadioChanged();

protected:
    void enableRegionViews(bool enable);

    QButtonGroup* m_previewModeButtons;

    PmCore* m_core;
    size_t m_matchIndex = 0;
};