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
    m_contentArea->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Maximum);

#if 0
    m_toggleAnimation = new QParallelAnimationGroup(this);
    m_toggleAnimation->addAnimation(
        new QPropertyAnimation(this, "minimumHeight"));
    m_toggleAnimation->addAnimation(
        new QPropertyAnimation(this, "maximumHeight"));
    m_toggleAnimation->addAnimation(
        new QPropertyAnimation(m_contentArea, "maximumHeight"));
#endif

    m_mainLayout = new QGridLayout;
    m_verticalSpacing = m_mainLayout->verticalSpacing();
    m_mainLayout->setVerticalSpacing(0);

    m_mainLayout->addWidget(
        m_toggleButton, 0, 0, 1, 1, Qt::AlignTop | Qt::AlignLeft);
    m_mainLayout->addWidget(
        m_topRightArea, 0, 1, 1, 1, Qt::AlignTop | Qt::AlignRight);
    m_mainLayout->addWidget(
        m_contentArea, 1, 0, 1, 2);
    setLayout(m_mainLayout);

    setFrameStyle(QFrame::Box);

    QObject::connect(
        m_toggleButton, &QToolButton::clicked,
        this, &PmSpoilerWidget::collapseToggled);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    collapse();
}

void PmSpoilerWidget::collapseToggled(bool checked)
{
	toggleExpand(checked);
}

void PmSpoilerWidget::toggleExpand(bool on)
{
	m_mainLayout->setVerticalSpacing(on ? m_verticalSpacing : 0);

    m_toggleButton->blockSignals(true);
    m_toggleButton->setChecked(on);
    m_toggleButton->setArrowType(
        on ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
    m_toggleButton->blockSignals(false);

    updateContentHeight();
    m_contentArea->setVisible(on);

#if 0
    m_toggleAnimation->setDirection(
        on ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    m_toggleAnimation->start();
#endif
}

int PmSpoilerWidget::maxContentHeight() const
{
	return m_contentArea->layout()->maximumSize().height();
}

void PmSpoilerWidget::updateContentHeight()
{
	bool contentOn = isExpanded();

	QMargins margins = m_mainLayout->contentsMargins();
	int marginsSpace = margins.top() + margins.bottom();

#if 0
	int contentMin = contentOn
        ? m_contentArea->layout()->minimumSize().height()
        : 0;
#endif

	int contentMax = contentOn ? maxContentHeight() : 0;

    int collapsed
        = qMax(m_toggleButton->sizeHint().height(),
               m_topRightArea->sizeHint().height()) + marginsSpace;
#if 0
    auto minHeightAnim = (QPropertyAnimation *)m_toggleAnimation->animationAt(0);
    minHeightAnim->setDuration(k_animationDuration);
    minHeightAnim->setStartValue(collapsed);
    minHeightAnim->setEndValue(contentMin + collapsed);

    auto maxHeightAnim = (QPropertyAnimation *)m_toggleAnimation->animationAt(1);
    maxHeightAnim->setDuration(k_animationDuration);
    maxHeightAnim->setStartValue(collapsed);
    maxHeightAnim->setEndValue(contentMax + collapsed);

    auto contentMaxAnim = (QPropertyAnimation *)m_toggleAnimation->animationAt(2);
    contentMaxAnim->setDuration(k_animationDuration);
    contentMaxAnim->setStartValue(0);
    contentMaxAnim->setEndValue(contentMax);
#endif

    m_contentArea->setMaximumHeight(contentMax);
    setMaximumHeight(contentMax + collapsed);

    //m_contentArea->setMinimumHeight(contentMin);
    //setMinimumHeight(contentMin + collapsed);
    //m_heightHint = contentMax + collapsed;
}

void PmSpoilerWidget::setContentLayout(QLayout *contentLayout)
{
    //delete m_contentArea->layout();
    m_contentArea->setLayout(contentLayout);

    if (isExpanded())
        updateContentHeight();
}

void PmSpoilerWidget::setTopRightLayout(QLayout *topRightLayout)
{
    m_topRightArea->setLayout(topRightLayout);
}

void PmSpoilerWidget::setTitle(const QString &title)
{
    m_toggleButton->setText(title);
}
