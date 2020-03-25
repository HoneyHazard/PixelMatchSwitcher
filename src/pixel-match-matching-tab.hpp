#pragma once

#include <QWidget>
#include <QPointer>

#include "pixel-match-structs.hpp"

class PmCore;
class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class OBSQTDisplay;
class QDoubleSpinBox;
class QButtonGroup;
class QStackedWidget;

/*!
 * \brief UI tab that shows match settings, UI preview and preview settings
 */
class PmMatchingTab : public QWidget
{
    Q_OBJECT

public:
    PmMatchingTab(PmCore *core, QWidget *parent);

signals:
    void sigOpenImage(QString filename);
    void sigNewUiConfig(PmConfigPacket);

private slots:
    void onBrowseButtonReleased();
    void onColorComboIndexChanged();
    void onImgSuccess(QString filename);
    void onImgFailed(QString filename);
    void onNewResults(PmResultsPacket results);
    void onConfigUiChanged();

private:
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize(
        const PmConfigPacket &config, const PmResultsPacket &results);

    void maskModeChanged(PmMaskMode mode, QColor color);

    QLineEdit* m_imgPathEdit;
    OBSQTDisplay *m_filterDisplay;
    QComboBox *m_maskModeCombo;
    QLabel *m_maskModeDisplay;
    QSpinBox *m_posXBox, *m_posYBox;
    QDoubleSpinBox *m_perPixelErrorBox;
    QDoubleSpinBox *m_totalMatchThreshBox;
    QLabel *m_matchResultDisplay;
    QButtonGroup *m_previewModeButtons;
    QStackedWidget *m_previewScaleStack;
    QComboBox *m_videoScaleCombo;
    QComboBox *m_regionScaleCombo;
    QComboBox *m_matchImgScaleCombo;

    QPointer<PmCore> m_core;
    PmResultsPacket m_prevResults;
};
