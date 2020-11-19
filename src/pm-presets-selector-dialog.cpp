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
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint
                | Qt::WindowCloseButtonHint)
{
    // all and none buttons
    QHBoxLayout *selAllNoneLayout = new QHBoxLayout;

    QPushButton *allButton = new QPushButton(obs_module_text("&All"), this);
    connect(allButton, &QPushButton::released, [this](){ setAll(true); });
    selAllNoneLayout->addWidget(allButton);

    QPushButton *noneButton = new QPushButton(obs_module_text("&None"), this);
    connect(noneButton, &QPushButton::released, [this]() { setAll(false); });
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
        new QPushButton(obs_module_text("&Cancel"), this);
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
    exec();
}


void PmPresetsSelectorDialog::setAll(bool value)
{
    for (QCheckBox *box : m_checkboxes) {
        box->setChecked(value);
    }
}

QList<std::string> PmPresetsSelectorDialog::selectedPresets() const
{
    QList<std::string> ret;
    for (QCheckBox *checkbox : m_checkboxes) {
        if (checkbox->isChecked()) {
            ret.append(checkbox->text().toUtf8().data());
        }
    }
    return ret;
}
