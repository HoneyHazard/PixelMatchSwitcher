#include "pm-match-image-dialog.hpp"

#include <QGraphicsPixmapItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

PmMatchImageDialog::PmMatchImageDialog(
    const QImage& image, QWidget* parent)
: QDialog(parent)
, m_image(image)
, m_scene(new QGraphicsScene(this))
, m_view(new QGraphicsView(m_scene, this))
{
    // image view
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
            this, &PmMatchImageDialog::onCancelReleased, Qt::QueuedConnection);

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

}

void PmMatchImageDialog::onCancelReleased()
{

}