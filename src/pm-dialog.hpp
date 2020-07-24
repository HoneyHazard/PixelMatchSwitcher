#pragma once

#include <QDialog>
#include <QPointer>

#include "pm-structs.hpp"

class PmCore;

class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);

signals:
    void sigCaptureStateChanged(PmCaptureState state, int x, int y);

protected slots:
    void onCaptureStateChanged(PmCaptureState state, int x, int y);

protected:
    void closeEvent(QCloseEvent*) override;

    QPointer<PmCore> m_core;
};
