#include "pm-presets-selector-dialog.hpp"
#include <obs-module.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QCheckBox>
#include <QPushButton>

PmPresetsSelectorDialog::PmPresetsSelectorDialog(
    QString title,
    const QList<std::string> &availablePresets,
    const QList<std::string> &selectedPresets,
    QWidget *parent)
: QDialog(parent)
{
    // all and none buttons
    QHBoxLayout *selAllNoneLayout = new QHBoxLayout;

    QPushButton *allButton = new QPushButton(obs_module_text("&All"), this);
    connect(allButton, &QPushButton::released,
            this, &PmPresetsSelectorDialog::onSelectAll);
    selAllNoneLayout->addWidget(allButton);

    QPushButton *noneButton = new QPushButton(obs_module_text("&None"), this);
    connect(noneButton, &QPushButton::released,
            this, &PmPresetsSelectorDialog::onSelectNone);
    selAllNoneLayout->addWidget(noneButton);

    // scroll area with preset checkboxes
    QVBoxLayout *checkboxLayout = new QVBoxLayout;

    m_checkboxes.reserve(availablePresets.size());
    for (const std::string &p : availablePresets) {
        QCheckBox *checkbox = new QCheckBox(p.data(), this);
        checkbox->setChecked(selectedPresets.contains(p));
        checkboxLayout->addWidget(checkbox);
        m_checkboxes.append(checkbox);
    }

    QWidget *checkboxesWidget = new QWidget(this);
    checkboxesWidget->setLayout(checkboxLayout);
    QScrollArea *checkboxScrollArea = new QScrollArea(this);
    checkboxScrollArea->setWidget(checkboxesWidget);

    // ok and cancel buttons
    QHBoxLayout *okCancelLayout = new QHBoxLayout;

    QPushButton *okButton = new QPushButton(obs_module_text("&OK"), this);
    connect(okButton, &QPushButton::released,
            this, &QDialog::accept);
    okCancelLayout->addWidget(okButton);

    QPushButton *cancelButton =
        new QPushButton(obs_module_text("Cancel"), this);
    connect(cancelButton, &QPushButton::released,
            this, &QDialog::reject);
    okCancelLayout->addWidget(cancelButton);

    // put it all together
    setWindowTitle(title);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(selAllNoneLayout);
    mainLayout->addWidget(checkboxScrollArea);
    mainLayout->addLayout(okCancelLayout);
    setLayout(mainLayout);
}


void PmPresetsSelectorDialog::onSelectAll()
{
    for (QCheckBox *box : m_checkboxes) {
        box->setChecked(true);
    }
}

void PmPresetsSelectorDialog::onSelectAll()
{
    for (QCheckBox *box : m_checkboxes) {
        box->setChecked(false);
    }
}
