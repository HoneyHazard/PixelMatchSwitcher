#include "pm-image-view.hpp"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>

PmImageView::PmImageView(QWidget* parent)
: QGraphicsView(new QGraphicsScene(parent), parent)
{
}

PmImageView::PmImageView(const QImage& image, QWidget* parent)
: PmImageView(parent)
{
    showImage(image);
}

void PmImageView::showImage(const QImage& image)
{
    auto viewScene = scene();

    viewScene->clear();
    QImage tileImage(":/res/images/tile.png");
    QGraphicsRectItem* bgItem = new QGraphicsRectItem(
        0, 0, image.width(), image.height());
    bgItem->setBrush(tileImage);
    viewScene->addItem(bgItem);

    QPixmap pixMap = QPixmap::fromImage(image);
    auto* item = new QGraphicsPixmapItem(pixMap);
    viewScene->addItem(item);
}