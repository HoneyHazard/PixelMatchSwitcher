#pragma once

#include <QGroupBox>
#include <QPointer>

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

class PmCore;

class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QStackedWidget;

/*!
 * \brief UI tab that shows match settings, UI preview and preview settings
 */
class PmMatchConfigWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmMatchConfigWidget(PmCore *core, QWidget *parent);

    size_t matchIndex() const { return m_matchIndex; }

signals:
    void sigChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg);
    void sigCaptureStateChanged(PmCaptureState capState, int x=-1, int y=-1);
    void sigRefreshMatchImage(size_t matchIdx);

protected slots:
    // core interaction
    void onNewMatchResults(size_t matchIdx, PmMatchResults results);
    void onChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg);
    void onSelectMatchIndex(size_t matchIndex, PmMatchConfig cfg);
    void onNewMultiMatchConfigSize(size_t sz);
    void onNewActiveFilter(PmFilterRef newAf);

    void onImgSuccess(size_t matchIndex, std::string filename, QImage img);
    void onImgFailed(size_t matchIndex, std::string filename);
    void onCaptureStateChanged(PmCaptureState capState, int x, int y);

    // UI element handlers
    void onOpenFileButtonReleased();
    void onOpenFolderButtonReleased();
    void onRefreshButtonReleased();

    void onPickColorButtonReleased();
    void onCaptureBeginReleased();
    void onCaptureAutomaskReleased();
    void onCaptureAcceptReleased();
    void onCaptureCancelReleased();

    // parse UI state into config
    void onConfigUiChanged();

protected:
    // UI assist
    static QColor toQColor(vec3 val);
    static vec3 toVec3(QColor val);
    void maskModeChanged(PmMaskMode mode, vec3 customColor);
    void roiRangesChanged(uint32_t baseWidth, uint32_t baseHeight);

protected:
    static const char* k_failedImgStr;

    size_t m_matchIndex = 0;
    size_t m_multiConfigSz = 0;

    QStackedWidget* m_buttonsStack;
    QPushButton* m_openFileButton;
    QPushButton* m_editFileButton;
    QPushButton* m_openFolderButton;
    QPushButton* m_refreshButton;

    QPushButton* m_captureBeginButton;
    QPushButton* m_captureAcceptButton;
    QPushButton* m_captureAutomaskButton;
    QPushButton* m_captureCancelButton;

    QLineEdit* m_imgPathEdit;
    QComboBox *m_maskModeCombo;
    QPushButton *m_pickColorButton;
    QLabel *m_maskModeDisplay;
    vec3 m_customColor;
    QSpinBox *m_posXBox, *m_posYBox;
    QDoubleSpinBox *m_perPixelErrorBox;
    QDoubleSpinBox *m_totalMatchThreshBox;

    PmCore *m_core;
    PmMatchResults m_prevResults;
};
