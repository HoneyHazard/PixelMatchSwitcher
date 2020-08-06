#pragma once

#include <QDialog>
#include <QPointer>
#include <QImage>

#include "pm-structs.hpp"

class PmCore;

/**
 * @brief Top-level aggregate module of all UI elements used by the plugin
 */
class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);

signals:
    void sigCaptureStateChanged(PmCaptureState state, int x=0, int y=0);
    void sigChangedMatchConfig(size_t matchIndex, PmMatchConfig config);

protected slots:
    void onCaptureStateChanged(PmCaptureState state, int x, int y);
    void onCapturedMatchImage(QImage matchImg, int roiLeft, int roiBottom);

protected:
    void closeEvent(QCloseEvent*) override;

    QPointer<PmCore> m_core;
};
