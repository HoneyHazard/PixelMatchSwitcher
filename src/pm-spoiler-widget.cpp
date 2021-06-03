#include "pm-spoiler-widget.hpp"

#include <QPropertyAnimation>

PmSpoilerWidget::PmSpoilerWidget(QWidget *parent)
: QFrame(parent)
{
    m_toggleButton = new QToolButton(this);
    m_toggleButton->setStyleSheet("QToolButton { border: none; }");
    m_toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggleButton->setArrowType(Qt::ArrowType::RightArrow);
    m_toggleButton->setText("--");
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(false);

    m_topRightArea = new QWidget(this);

    m_contentArea = new QWidget(this);
    m_contentArea->setMaximumHeight(0);
    m_contentArea->setMinimumHeight(0);

    m_toggleAnimation = new QParallelAnimationGroup(this);
    //m_toggleAnimation->addAnimation(
    //    new QPropertyAnimation(this, "minimumHeight"));
    m_toggleAnimation->addAnimation(
        new QPropertyAnimation(this, "maximumHeight"));
    m_toggleAnimation->addAnimation(
        new QPropertyAnimation(m_contentArea, "maximumHeight"));

    m_mainLayout = new QGridLayout;
    m_verticalSpacing = m_mainLayout->verticalSpacing();
    m_mainLayout->setVerticalSpacing(0);
    //m_mainLayout->setContentsMargins(0, 0, 0, 0);

    m_mainLayout->addWidget(
        m_toggleButton, 0, 0, 1, 1, Qt::AlignTop | Qt::AlignLeft);
    m_mainLayout->addWidget(
        m_topRightArea, 0, 1, 1, 1, Qt::AlignTop | Qt::AlignRight);
    m_mainLayout->addWidget(m_contentArea, 1, 0, 1, 2);
    setLayout(m_mainLayout);

    setFrameStyle(QFrame::Box);

    QObject::connect(
        m_toggleButton, &QToolButton::clicked,
        this, &PmSpoilerWidget::collapseToggled);
}

void PmSpoilerWidget::collapseToggled(bool checked)
{
	expand(checked);
}

void PmSpoilerWidget::expand(bool on)
{
    m_toggleButton->setArrowType(
        on ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
    m_toggleAnimation->setDirection(
        on ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    m_toggleAnimation->start();

    m_toggleButton->blockSignals(true);
    m_toggleButton->setChecked(on);
    m_toggleButton->blockSignals(false);

    m_mainLayout->setVerticalSpacing(on ? m_verticalSpacing : 0);
}

void PmSpoilerWidget::updateContentHeight(int contentHeight)
{
    int collapsedHeight = qMax(m_toggleButton->sizeHint().height(),
                               m_topRightArea->sizeHint().height());

    #if 0
    for (int i = 0; i < m_toggleAnimation->animationCount() - 1; ++i) {
        QPropertyAnimation *spoilerAnimation =
            static_cast<QPropertyAnimation *>(
                m_toggleAnimation->animationAt(i));
        spoilerAnimation->setDuration(k_animationDuration);
        spoilerAnimation->setStartValue(contentHeight);
        spoilerAnimation->setEndValue(collapsedHeight + contentHeight);
    }
    #endif
    QPropertyAnimation *contentAnimation =
        static_cast<QPropertyAnimation *>(
            m_toggleAnimation->animationAt(
                m_toggleAnimation->animationCount() - 1));
    contentAnimation->setDuration(k_animationDuration);
    contentAnimation->setStartValue(0);
    contentAnimation->setEndValue(contentHeight);
}

void PmSpoilerWidget::setContentLayout(QLayout *contentLayout)
{
    //delete m_contentArea->layout();
    m_contentArea->setLayout(contentLayout);

    auto contentHeight = contentLayout->sizeHint().height();
    //auto contentHeight = contentLayout->minimumSize().height();
    updateContentHeight(contentHeight);
}

void PmSpoilerWidget::setTopRightLayout(QLayout *topRightLayout)
{
    m_topRightArea->setLayout(topRightLayout);
}

void PmSpoilerWidget::setTitle(const QString &title)
{
    m_toggleButton->setText(title);
}
