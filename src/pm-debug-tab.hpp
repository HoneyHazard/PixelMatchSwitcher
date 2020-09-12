#pragma once

#include <QDialog>


class QLabel;
class QTextEdit;
class PmCore;

class PmDebugTab : public QDialog
{
    Q_OBJECT

public:
    PmDebugTab(PmCore *pixelMatcher, QWidget *parent);

private slots:
    void scenesInfoReleased();
    void periodicUpdate();

private:
    PmCore *m_core;

    QLabel* m_filterModeDisplay;
    QLabel *m_activeFilterDisplay;
    QLabel *m_sourceResDisplay;
    QLabel *m_filterDataResDisplay;
    QLabel *m_matchCountDisplay;
    QLabel* m_captureStateDisplay;
    QLabel* m_previewModeDisplay;
    QTextEdit *m_textDisplay;
};
