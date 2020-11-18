#pragma once

#include <QDialog>
#include <string>
#include "pm-structs.hpp"

/**
 * @brief Shows prompt to replace, rename, or skip importing a preset when there
 *        is already a preset already with the same name
 */
class PmPresetExistsDialog : public QDialog
{
    Q_OBJECT

public:
    PmPresetExistsDialog(
        const std::string &presetName, bool askApplyAll, QWidget *parent);
    PmDuplicateNameReaction choice() const { return m_choice; } 
    bool applyToAll() const { return m_applyToAll; }

protected slots:
    void renameReleased();
    void replaceReleased();
    void skipReleased();
    void abortReleased();

    void abortReleased();
    void applyAllToggled(bool value);

protected:
    PmDuplicateNameReaction m_choice = PmDuplicateNameReaction::Undefined;
    bool m_applyToAll = false;
};
