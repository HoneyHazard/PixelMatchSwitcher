#pragma once

#include <QDialog>

class PixelMatcher;
class QLabel;
class OBSQTDisplay;

class PixelMatchDialog : public QDialog
{
    Q_OBJECT

public:
    PixelMatchDialog(PixelMatcher *pixelMatcher, QWidget *parent);

private slots:
    void onNewFrameImage();

private:
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);

    QLabel *m_testLabel;
    OBSQTDisplay *m_filterDisplay;
    PixelMatcher *m_pixelMatcher;
};
