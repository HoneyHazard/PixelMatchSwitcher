#pragma once

#include <QDialog>
#include <QPointer>

#include "pixel-match-structs.hpp"

class PmCore;
class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class OBSQTDisplay;
class QDoubleSpinBox;

class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);
    ~PmDialog() {}

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
    enum ColorMode : int { GreenMode=0, MagentaMode=1, BlackMode=2,
                           AlphaMode=3, CustomClrMode=4 };
    enum DisplayMode { MatchImage, FilterOutput, MatchArea };

    static void drawPreview(void *data, uint32_t cx, uint32_t cy);

    void colorModeChanged(ColorMode mode, QColor color);

    QLineEdit* m_imgPathEdit;
    OBSQTDisplay *m_filterDisplay;
    QComboBox *m_colorModeCombo;
    QLabel *m_colorModeDisplay;
    QSpinBox *m_posXBox, *m_posYBox;
    QDoubleSpinBox *m_totalMatchThreshBox;
    QLabel *m_matchResultDisplay;

    QPointer<PmCore> m_core;
    PmResultsPacket m_prevResults;
};
