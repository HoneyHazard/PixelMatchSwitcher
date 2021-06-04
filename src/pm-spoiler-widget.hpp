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
    static const int k_animationDuration = 300;

    PmSpoilerWidget(QWidget *parent = nullptr);
    void setContentLayout(QLayout *contentLayout);
    void setTopRightLayout(QLayout *topRightLayout);
    void setTitle(const QString &title);

    virtual void toggleExpand(bool on);
    void expand() { toggleExpand(true); }
    void collapse() { toggleExpand(false); }
    bool isExpanded() const { return m_toggleButton->isChecked(); }

    QSize sizeHint() const override;

protected slots:
    void collapseToggled(bool checked);

protected:
    virtual void updateContentHeight();

    QGridLayout* m_mainLayout;
    QToolButton* m_toggleButton;
    //QParallelAnimationGroup* m_toggleAnimation;
    QWidget *m_contentArea;
    QWidget *m_topRightArea;

    int m_verticalSpacing = 0;
    int m_marginsSpace = 0;
    int m_heightHint = 0;
};
