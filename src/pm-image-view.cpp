#include "pm-image-view.hpp"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QShowEvent>

PmImageView::PmImageView(QWidget* parent)
: QGraphicsView(new QGraphicsScene(parent), parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    auto* imageItem = new QGraphicsPixmapItem(pixMap);
    viewScene->addItem(imageItem);

    m_activeItem = imageItem;
}

void PmImageView::fixGeometry()
{
    if (m_activeItem) {
        fitInView(m_activeItem);
    }
}

void PmImageView::resizeEvent(QResizeEvent* resizeEvent)
{
    if (isVisible())
        fixGeometry();
}

void PmImageView::showEvent(QShowEvent* se)
{
    fixGeometry();
}
