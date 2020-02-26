#pragma once

#include <QDialog>

class PmCore;
class QLabel;
class OBSQTDisplay;

class PixelMatchDialog : public QDialog
{
    Q_OBJECT

public:
    PixelMatchDialog(PmCore *pixelMatcher, QWidget *parent);

private slots:
    void onNewFrameImage();

private:
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);

    QLabel *m_testLabel;
    OBSQTDisplay *m_filterDisplay;
    PmCore *m_pixelMatcher;
};
