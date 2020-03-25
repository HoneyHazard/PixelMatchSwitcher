#pragma once

#include "pixel-match-structs.hpp"

#include <QWidget>
#include <QPointer>

class PmCore;

class QComboBox;

/*!
 * \brief UI tab that shows settings for active and inactives scens, transitions
 */
class PmScenesTab : public QWidget
{
    Q_OBJECT

public:
    PmScenesTab(PmCore *core, QWidget *parent);

signals:
    void sigSceneConfigChanged(PmSceneConfig);

private slots:
    void onScenesChanged(PmScenes);
    void onConfigUiChanged();

protected:
    QComboBox *m_matchSceneCombo;
    QComboBox *m_noMatchSceneCombo;
    QComboBox *m_transitionCombo;
    // TODO: scene-to-scene custom transitions

    QPointer<PmCore> m_core;
};
