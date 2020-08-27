#include "pm-match-image-dialog.hpp"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

PmMatchImageDialog::PmMatchImageDialog(
    const QImage& image, QWidget* parent)
: QDialog(parent)
, m_image(image)
, m_scene(new QGraphicsScene(this))
, m_view(new QGraphicsView(m_scene, this))
{
    setWindowTitle(obs_module_text("Match Image Preview"));

    // scene
    QImage tileImage(":/res/images/tile.png");
    QGraphicsRectItem* bgItem = new QGraphicsRectItem(
        0, 0, image.width(), image.height());
    bgItem->setBrush(tileImage);
    m_scene->addItem(bgItem);

    QPixmap pixMap = QPixmap::fromImage(image);
    auto* item = new QGraphicsPixmapItem(pixMap);
    m_scene->addItem(item);

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
    mainLayout->addWidget(m_view);
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
            m_saveLocation = saveFilename.toUtf8();
            accept();
        } else {
            QString errMsg = QString(
                obs_module_text("Unable to save file: %1")).arg(saveFilename);
            QMessageBox::critical(
                this, obs_module_text("Error"), errMsg);
        }
    }
}

