#pragma once

#include <QDialog>
#include <QPalette>

#include "pixel-match-structs.hpp"

class PmCore;
class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class OBSQTDisplay;

class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);

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

    PmCore *m_core;
    PmResultsPacket m_prevResults;
};
