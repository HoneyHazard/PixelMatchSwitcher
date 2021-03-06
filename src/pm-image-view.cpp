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
    setRenderHints(QPainter::RenderHints()); // no-antialiasing
}

void PmImageView::showMessage(const QString &message, QColor color)
{
    auto viewScene = scene();
    viewScene->clear();

    QImage colorBarsImage(":/res/images/SMPTE_Color_Bars.png");
    QPixmap pixMap = QPixmap::fromImage(colorBarsImage);
    auto* imageItem = new QGraphicsPixmapItem(pixMap);
    viewScene->addItem(imageItem);

    int backdropWidth = int(colorBarsImage.width() * 0.8f);
    int backdropHeight = int(colorBarsImage.height() * 0.2f);

    QGraphicsRectItem* backdropItem = new QGraphicsRectItem(
        0, 0, backdropWidth, backdropHeight*2.25f, imageItem);
    backdropItem->setPen(QPen(color, 3.f));
    backdropItem->setBrush(palette().window());
    backdropItem->setPos(imageItem->boundingRect().center() 
                       - backdropItem->boundingRect().center());

    QGraphicsTextItem* textItem = new QGraphicsTextItem(imageItem);
    textItem->setHtml(QString("<center>") + message + QString("</center>"));
    int fontSz = int(backdropHeight * 0.2f);
    textItem->setFont(QFont(parentWidget()->font().family(), fontSz));
    textItem->setTextWidth(backdropWidth);
    textItem->setDefaultTextColor(color);
    textItem->setPos(imageItem->boundingRect().center()
                   - textItem->boundingRect().center());

    m_activeItem = imageItem;
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
}

void PmImageView::fixGeometry()
{
    if (m_activeItem) {
        fitInView(m_activeItem, Qt::AspectRatioMode::KeepAspectRatio);
    }
}

void PmImageView::resizeEvent(QResizeEvent* resizeEvent)
{
    if (isVisible())
        fixGeometry();
    QGraphicsView::resizeEvent(resizeEvent);
}

void PmImageView::wheelEvent(QWheelEvent* wheelEvent)
{
    // override to do nothing to block the wheel scrolling
    wheelEvent->accept();
}
