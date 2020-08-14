#pragma once

#include <QDialog>
#include <QImage>
#include <QGraphicsView>

#include <string>

#include "pm-structs.hpp"

/**
 * @brief Widget for presenting a match image to user to initiate saving or
 *        rejection
 */
class PmMatchImageDialog : public QDialog
{
    Q_OBJECT

public:
    PmMatchImageDialog(const QImage& image, QWidget *parent = nullptr);
    const std::string& saveLocation() const { return m_saveLocation; }

protected:
    std::string m_saveLocation;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    QImage m_image;
};