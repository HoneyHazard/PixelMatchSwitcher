#pragma once

#include <QDialog>

class PixelMatcher;
class QLabel;

class PixelMatchDialog : public QDialog
{
    Q_OBJECT

public:
    PixelMatchDialog(PixelMatcher *pixelMatcher, QWidget *parent);

private slots:
    void onNewFrameImage();

private:
    QLabel *m_testLabel;
    PixelMatcher *m_pixelMatcher;
};
