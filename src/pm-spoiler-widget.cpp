#include "pm-spoiler-widget.hpp"

#include <QPropertyAnimation>

PmSpoilerWidget::PmSpoilerWidget(
    const QString &title, const int animationDuration, QWidget *parent)
: QWidget(parent)
, m_animationDuration(animationDuration)
{
	m_toggleButton = new QToolButton(this);
    m_toggleButton->setStyleSheet("QToolButton { border: none; }");
	m_toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggleButton->setArrowType(Qt::ArrowType::RightArrow);
	m_toggleButton->setText(title);
    m_toggleButton->setCheckable(true);
	m_toggleButton->setChecked(false);

    //m_headerLine.setFrameShape(QFrame::HLine);
    //m_headerLine.setFrameShadow(QFrame::Sunken);
	m_headerLine = new QFrame(this);
	m_headerLine->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Maximum);

    //m_contentArea.setStyleSheet(
    //    "QScrollArea { background-color: white; border: none; }");
    // start out collapsed
	m_contentArea = new QScrollArea(this);
	m_contentArea->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Minimum);
	m_contentArea->setMaximumHeight(0);
	m_contentArea->setMinimumHeight(0);
    // let the entire widget grow and shrink with its content

    m_toggleAnimation = new QParallelAnimationGroup(this);
	m_toggleAnimation->addAnimation(
        new QPropertyAnimation(this, "minimumHeight"));
    m_toggleAnimation->addAnimation(
        new QPropertyAnimation(this, "maximumHeight"));
	m_toggleAnimation->addAnimation(
        new QPropertyAnimation(m_contentArea, "maximumHeight"));
    // don't waste space

    m_mainLayout = new QGridLayout;
    m_mainLayout->setVerticalSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    int row = 0;
    m_mainLayout->addWidget(m_toggleButton, row, 0, 1, 1, Qt::AlignLeft);
    m_mainLayout->addWidget(m_headerLine, row++, 2, 1, 1);
    m_mainLayout->addWidget(m_contentArea, row, 0, 1, 3);
    setLayout(m_mainLayout);

    QObject::connect(
        m_toggleButton, &QToolButton::clicked,
        [this](const bool checked) {
		    auto contentHeight =
			    m_contentArea->layout()->sizeHint().height();
		    updateContentHeight(contentHeight);

            m_toggleButton->setArrowType(checked
                ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
            m_toggleAnimation->setDirection(checked
                ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
            m_toggleAnimation->start();
			//updateGeometry();
        });
}

void PmSpoilerWidget::updateContentHeight(int contentHeight)
{
	//const int collapsedHeight =
	//	sizeHint().height() - m_contentArea->maximumHeight();
	int collapsedHeight = m_headerLine->height();
	for (int i = 0; i < m_toggleAnimation->animationCount() - 1; ++i) {
		QPropertyAnimation *spoilerAnimation =
			static_cast<QPropertyAnimation *>(
				m_toggleAnimation->animationAt(i));
		spoilerAnimation->setDuration(m_animationDuration);
		spoilerAnimation->setStartValue(contentHeight);
		spoilerAnimation->setEndValue(collapsedHeight + contentHeight);
	}
	QPropertyAnimation *contentAnimation =
		static_cast<QPropertyAnimation *>(
			m_toggleAnimation->animationAt(
				m_toggleAnimation->animationCount() - 1));
	contentAnimation->setDuration(m_animationDuration);
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

void PmSpoilerWidget::setTitle(const QString &title)
{
    m_toggleButton->setText(title);
}
