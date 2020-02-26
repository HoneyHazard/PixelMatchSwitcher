#pragma once

#include <QDialog>
#include <QPalette>

class PmCore;
class QLabel;
class QLineEdit;
class QComboBox;
class OBSQTDisplay;

class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);

private slots:
    void onBrowseButtonReleased();
    void onColorComboIndexChanged();

private:
    enum ColorMode : int { GreenMode=0, MagentaMode=1, BlackMode=2,
                           AlphaMode=3, CustomClrMode=4 };
    enum DisplayMode { MatchImage, FilterOutput, MatchArea };

    static void drawPreview(void *data, uint32_t cx, uint32_t cy);

    void colorChanged(ColorMode mode, QColor color);

    QLineEdit* m_imgPathEdit;
    OBSQTDisplay *m_filterDisplay;
    QComboBox *m_colorModeCombo;
    QLabel *m_colorModeDisplay;

    PmCore *m_core;
};
