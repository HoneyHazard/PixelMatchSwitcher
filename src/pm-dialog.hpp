#pragma once

#include <QDialog>
#include <QImage>

#include "pm-structs.hpp"

class PmCore;
class PmPresetsWidget;
class QTabWidget;

/**
 * @brief Top-level aggregate module of all UI elements used by the plugin
 */
class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);

signals:
    void sigCaptureStateChanged(PmCaptureState state, int x=-1, int y=-1);
    void sigMatchConfigChanged(size_t matchIndex, PmMatchConfig config);

protected slots:
    void onCaptureStateChanged(PmCaptureState state, int x, int y);
    void onMatchImageCaptured(QImage matchImg, int roiLeft, int roiBottom);
    void onShowException(std::string caption, std::string descr);

protected:
    void closeEvent(QCloseEvent* closeEvent) override;

    PmCore* m_core;
    PmPresetsWidget* m_presetsWidget;
};
