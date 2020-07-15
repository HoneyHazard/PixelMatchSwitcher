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
    // signals sent to core
    void sigRunningEnabledChanged(bool enable);
    void sigSwitchingEnabledChanged(bool enable);

protected slots:
    // core event handlers
    void onRunningEnabledChanged(bool enable);
    void onSwitchingEnabledChanged(bool enable);
    
protected:
    static const char *k_aboutText;

    PmCore* m_core;

    QCheckBox* m_runningCheckbox;
    QCheckBox* m_switchingCheckbox;
};