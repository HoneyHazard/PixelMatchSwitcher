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
    void scenesInfoReleased();
    void periodicUpdate();

private:
    PixelMatcher *m_pixelMatcher;

    QLabel *m_filtersStatusDisplay;
    QLabel *m_activeFilterDisplay;
    QLabel *m_sourceResDisplay;
    QLabel *m_filterDataResDisplay;
    QLabel *m_matchCountDisplay;
    QTextEdit *m_textDisplay;
};
