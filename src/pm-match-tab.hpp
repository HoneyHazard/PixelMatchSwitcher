#pragma once

#include <QWidget>
#include <QPointer>

#include "pm-structs.hpp"

class PmCore;

class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class OBSQTDisplay;
class QDoubleSpinBox;
class QButtonGroup;
class QStackedWidget;

/*!
 * \brief UI tab that shows match settings, UI preview and preview settings
 */
class PmMatchTab : public QWidget
{
    Q_OBJECT

public:
    PmMatchTab(PmCore *core, QWidget *parent);
    ~PmMatchTab() override;

signals:
    void sigNewUiConfig(PmMatchConfig);

private slots:
    void onBrowseButtonReleased();
    void onColorComboIndexChanged();
    void onImgSuccess(std::string filename);
    void onImgFailed(std::string filename);
    void onNewMatchResults(PmMatchResults results);
    void onConfigUiChanged();
    void onDestroy(QObject *obj);

protected:
    virtual void closeEvent(QCloseEvent *e) override;

private:
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize(
        const PmMatchConfig &config, const PmMatchResults &results);

    void maskModeChanged(PmMaskMode mode, QColor color);

    QLineEdit* m_imgPathEdit;
    OBSQTDisplay *m_filterDisplay;
    QComboBox *m_maskModeCombo;
    QLabel *m_maskModeDisplay;
    QSpinBox *m_posXBox, *m_posYBox;
    QDoubleSpinBox *m_perPixelErrorBox;
    QDoubleSpinBox *m_totalMatchThreshBox;
    QLabel *m_matchResultDisplay;
    QButtonGroup *m_previewModeButtons;
    QStackedWidget *m_previewScaleStack;
    QComboBox *m_videoScaleCombo;
    QComboBox *m_regionScaleCombo;
    QComboBox *m_matchImgScaleCombo;
    QLabel *m_notifyLabel;

    QPointer<PmCore> m_core;
    PmMatchResults m_prevResults;
    bool m_rendering = false; // safeguard against deletion while rendering in obs render thread
};
