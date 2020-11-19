#include "pm-preset-exists-dialog.hpp"
#include <obs-module.h>

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

PmPresetExistsDialog::PmPresetExistsDialog(
    const std::string &presetName, bool askApplyAll, QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint
                | Qt::WindowCloseButtonHint)
{
    setWindowTitle(obs_module_text("Preset exists"));

    QVBoxLayout *mainLayout = new QVBoxLayout;

    // label
    QLabel *label = new QLabel(
        QString(obs_module_text(
            "A different version of preset \"%1\" already "
            "exists in your configuration\n\n"
            "How do you want to import \"%1\"?\n")).arg(presetName.data()),
            this);
    mainLayout->addWidget(label);

    // buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;

    QPushButton *renameButton = new QPushButton(
        obs_module_text("Rename"), this);
    connect(renameButton, &QPushButton::released,
            this, &PmPresetExistsDialog::renameReleased);
    buttonLayout->addWidget(renameButton);

    QPushButton* replaceButton = new QPushButton(
        obs_module_text("Replace"), this);
    connect(replaceButton, &QPushButton::released,
            this, &PmPresetExistsDialog::replaceReleased);
    buttonLayout->addWidget(replaceButton);

    QPushButton* skipButton = new QPushButton(
        obs_module_text("Skip"), this);
    connect(skipButton, &QPushButton::released,
            this, &PmPresetExistsDialog::skipReleased);
    buttonLayout->addWidget(skipButton);

    QPushButton *abortButton = new QPushButton(obs_module_text("Abort"), this);
    connect(abortButton, &QPushButton::released, this,
	    &PmPresetExistsDialog::abortReleased);
    buttonLayout->addWidget(abortButton);

    mainLayout->addLayout(buttonLayout);

    // apply to all
    if (askApplyAll) {
        QCheckBox* applyToAllCheckBox = new QCheckBox(
            obs_module_text("Apply to All Preset Collisions"), this);
	    applyToAllCheckBox->setChecked(m_applyToAll);
        connect(applyToAllCheckBox, &QCheckBox::toggled,
                this, &PmPresetExistsDialog::applyAllToggled);
        mainLayout->addWidget(applyToAllCheckBox, 0, Qt::AlignCenter);
    }

    setLayout(mainLayout);
    exec();
}

void PmPresetExistsDialog::renameReleased()
{
    m_choice = PmDuplicateNameReaction::Rename;
    accept();
}

void PmPresetExistsDialog::replaceReleased()
{
    m_choice = PmDuplicateNameReaction::Replace;
    accept();
}

void PmPresetExistsDialog::skipReleased()
{
    m_choice = PmDuplicateNameReaction::Skip;
    accept();
}

void PmPresetExistsDialog::abortReleased()
{
    m_choice = PmDuplicateNameReaction::Abort;
    accept();
}

void PmPresetExistsDialog::applyAllToggled(bool value)
{
    m_applyToAll = value;
}
