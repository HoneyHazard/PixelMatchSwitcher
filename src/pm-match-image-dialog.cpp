#include "pm-match-image-dialog.hpp"

#include <QGraphicsPixmapItem>
#include <QHBoxLayout>
#include <QVBoxLayout>

PmMatchImageDialog::PmMatchImageDialog(
    const QImage& image, QWidget* parent)
: QDialog(parent)
, m_image(image)
, m_scene(new QGraphicsScene(this))
, m_view(new QGraphicsView(m_scene, this))
{
    QPixmap pixMap = QPixmap::fromImage(image);
    auto* item = new QGraphicsPixmapItem(pixMap);
    m_scene->addItem(item);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_view);
    setLayout(mainLayout);

    setMinimumSize(200, 200);
}