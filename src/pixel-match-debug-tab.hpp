#pragma once

#include <QWidget>

class QLabel;
class PixelMatcher;

class PixelMatchDebugTab : public QWidget
{
    Q_OBJECT

public:
    PixelMatchDebugTab(PixelMatcher *pixelMatcher, QWidget *parent);

private slots:
    void findReleased();

private:
    PixelMatcher *m_pixelMatcher;

    QLabel *m_statusDisplay;
};
