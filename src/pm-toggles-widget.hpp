#pragma once

#include <QGroupBox>

class PmCore;
class QCheckBox;

class PmTogglesWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmTogglesWidget(PmCore* core, QWidget* parent);

signals:
    void sigRunningEnabledChanged(bool enable);
    void sigSwitchingEnabledChanged(bool enable);

protected slots:
    void onRunningEnabledChanged(bool enable);
    void onSwitchingEnabledChanged(bool enable);

protected:
    PmCore* m_core;

    QCheckBox* m_runningCheckbox;
    QCheckBox* m_switchingCheckbox;
};