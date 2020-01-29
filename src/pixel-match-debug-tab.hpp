#pragma once

#include <QWidget>

class QLabel;
class QTextEdit;
class PixelMatcher;

class PixelMatchDebugTab : public QWidget
{
    Q_OBJECT

public:
    PixelMatchDebugTab(PixelMatcher *pixelMatcher, QWidget *parent);

private slots:
    void enumReleased();

private:
    PixelMatcher *m_pixelMatcher;

    QLabel *m_statusDisplay;
    QTextEdit *m_textDisplay;
};
