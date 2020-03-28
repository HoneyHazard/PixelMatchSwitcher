#pragma once

#include "pm-structs.hpp"

#include <QWidget>
#include <QPointer>

class PmCore;

class QCheckBox;
class QComboBox;

/*!
 * \brief UI tab that shows settings for active and inactives scens, transitions
 */
class PmSwitchTab : public QWidget
{
    Q_OBJECT

public:
    PmSwitchTab(PmCore *core, QWidget *parent);

signals:
    void sigSceneConfigChanged(PmSwitchConfig);

private slots:
    void onScenesChanged(PmScenes);
    void onMatchSceneChanged();
    void onNoMatchSceneChanged();
    void onEnabledChanged();

protected:
    static const char *k_dontSwitchName;

    static OBSWeakSource pickScene(
        const PmScenes &scenes, const OBSWeakSource &another = nullptr);
    static void setSelectedScene(QComboBox *combo, OBSWeakSource &scene);
    static OBSWeakSource findScene(const PmScenes &scenes, const QString &name);

    QCheckBox *m_enabledCheck;
    QComboBox *m_matchSceneCombo;
    QComboBox *m_noMatchSceneCombo;
    QComboBox *m_transitionCombo;
    // TODO: scene-to-scene custom transitions

    QPointer<PmCore> m_core;
};
