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
    void onMatchSceneChanged();
    void onNoMatchSceneChanged();

protected:
    static OBSWeakSource pickScene(
        const PmScenes &scenes, const OBSWeakSource &another = nullptr);
    static void setSelectedScene(QComboBox *combo, OBSWeakSource &scene);
    static OBSWeakSource findScene(const PmScenes &scenes, const QString &name);

    QComboBox *m_matchSceneCombo;
    QComboBox *m_noMatchSceneCombo;
    QComboBox *m_transitionCombo;
    // TODO: scene-to-scene custom transitions

    QPointer<PmCore> m_core;
};
