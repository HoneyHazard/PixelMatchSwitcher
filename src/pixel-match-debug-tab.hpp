#pragma once

#include <QWidget>

class QLabel;
class QTextEdit;
class PmCore;

class PixelMatchDebugTab : public QWidget
{
    Q_OBJECT

public:
    PixelMatchDebugTab(PmCore *pixelMatcher, QWidget *parent);

private slots:
    void scenesInfoReleased();
    void periodicUpdate();

private:
    PmCore *m_pixelMatcher;

    QLabel *m_filtersStatusDisplay;
    QLabel *m_activeFilterDisplay;
    QLabel *m_sourceResDisplay;
    QLabel *m_filterDataResDisplay;
    QLabel *m_matchCountDisplay;
    QTextEdit *m_textDisplay;
};
