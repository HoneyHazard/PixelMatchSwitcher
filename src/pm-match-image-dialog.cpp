#include "pm-match-image-dialog.hpp"
#include "pm-image-view.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

PmMatchImageDialog::PmMatchImageDialog(
    const QImage& image, QWidget* parent)
: QDialog(parent)
, m_image(image)
{
    setWindowTitle(obs_module_text("Match Image Preview"));

    // image preview
    PmImageView* imageView = new PmImageView(image, this);

    // save buttons
    QPushButton* saveButton = new QPushButton(
        obs_module_text("Save"), this);
    connect(saveButton, &QPushButton::released,
            this, &PmMatchImageDialog::onSaveReleased, Qt::QueuedConnection);

    QPushButton* cancelButton = new QPushButton(
        obs_module_text("Cancel"), this);
    connect(cancelButton, &QPushButton::released,
            this, &QDialog::reject, Qt::QueuedConnection);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addWidget(cancelButton);

    // put it all together
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(imageView);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setMinimumSize(200, 200);
}

void PmMatchImageDialog::onSaveReleased()
{
    QString saveFilename = QFileDialog::getSaveFileName(
        this,
        obs_module_text("Save New Match Image"),
        "",
        PmConstants::k_imageFilenameFilter);

    if (saveFilename.size()) {
        bool ok = m_image.save(saveFilename);
        if (ok) {
            m_saveLocation = std::string(saveFilename.toUtf8());
            accept();
        } else {
            QString errMsg = QString(
                obs_module_text("Unable to save file: %1")).arg(saveFilename);
            QMessageBox::critical(
                this, obs_module_text("Error"), errMsg);
        }
    }
}
