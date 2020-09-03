#pragma once

#include <QGraphicsView>

/**
 * @brief Viewer widget for displaying simple graphics scenes containing match 
 *        image or messages for user
 * 
 *  In the future can be expanded to also be a simple match image editor
 */
class PmImageView : public QGraphicsView
{
    Q_OBJECT

public:
    PmImageView(QWidget* parent = nullptr);
    PmImageView(const QImage& image, QWidget* parent = nullptr);
    void showImage(const QImage& image);

protected:
    QGraphicsView* m_view;
};