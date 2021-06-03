#pragma once

/**
 * @brief Spoiler widget allows for convenient collapsing of its layour content
 * 
 * Taken from post by x squared at:
 * https://stackoverflow.com/questions/32476006/how-to-make-an-expandable-collapsable-section-widget-in-qt
 */

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>

class PmSpoilerWidget : public QFrame
{
    Q_OBJECT

public:
    explicit PmSpoilerWidget(
        const QString &title = "", const int animationDuration = 300,
        QWidget *parent = 0);
    void setContentLayout(QLayout *contentLayout);
    void setTitle(const QString &title);

protected:
    void updateContentHeight(int height);

    QGridLayout* m_mainLayout;
    QToolButton* m_toggleButton;
    //QFrame* m_headerLine;
    QParallelAnimationGroup* m_toggleAnimation;
    //QScrollArea* m_contentArea;
    QWidget *m_contentArea;
    int m_animationDuration = 300;
};
