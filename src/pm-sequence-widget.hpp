#pragma once

#include <QWidget>

class QPushButton;
class QLabel;
class PmCore;

class PmSequenceWidget : public QWidget
{
    Q_OBJECT

public:
    PmSequenceWidget(PmCore *core, QWidget *parent);
	void setSequenceId(int seqId);

signals:
    void sigSequenceActivate(int seqId);
	void sigSequencePause(int seqId);
    void sigSequenceReset(int seqId);

protected slots:
    void onSequenceStateChanged(int seqId);
    void onStartResumeReleased();
    void onResetReleased();

protected:
    QPushButton* prepareButton(
        const char *tooltip, const char *icoPath, char *themeId);

	QPushButton *m_startResumeButton;
	QPushButton *m_resetButton;
	//QPushButton *m_importButton;
	QLabel *m_statusDisplay;

    PmCore *m_core;
	int m_sequenceId = -1;
};
