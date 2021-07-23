#include "pm-sequence-widget.hpp"
#include "pm-core.hpp"
#include <obs-module.h>
#include <sstream>

#include <QPushButton>
#include <QVariant>
#include <QHBoxLayout>
#include <QLabel>

PmSequenceWidget::PmSequenceWidget(PmCore *core, QWidget *parent)
: QWidget(parent)
, m_core(core)
{
    m_startResumeButton = prepareButton(obs_module_text("Start"),
        ":/res/images/media/media_play.svg", "playIcon");
	m_resetButton = prepareButton(obs_module_text("Reset"),
        ":/res/images/revert.svg", "revertIcon");
    m_statusDisplay = new QLabel(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_startResumeButton);
    mainLayout->addWidget(m_resetButton);
    mainLayout->addWidget(m_statusDisplay);

    // core -> this
    auto qc = Qt::QueuedConnection;
    connect(m_core, &PmCore::sigSequenceStateChanged,
            this, &PmSequenceWidget::onSequenceStateChanged, qc);

    // this -> core
    connect(this, &PmSequenceWidget::sigSequenceActivate,
            m_core, &PmCore::onSequenceActivate, qc);
    connect(this, &PmSequenceWidget::sigSequencePause,
            m_core, &PmCore::onSequencePause, qc);
    connect(this, &PmSequenceWidget::sigSequenceReset,
            m_core, &PmCore::onSequenceReset, qc);

    // local UI connections
    connect(m_startResumeButton,  &QPushButton::released,
            this, &PmSequenceWidget::onStartResumeReleased);
    connect(m_resetButton, &QPushButton::released,
            this, &PmSequenceWidget::onResetReleased);
}

void PmSequenceWidget::onSequenceStateChanged(int seqId)
{
	if (seqId != m_sequenceId) return;

    bool isActive = m_core->sequenceIsActive(seqId);
    size_t currMatchIdx = m_core->sequenceCurrMatchIndex(seqId);
    std::string matchLabel = m_core->matchConfigLabel(seqId);

	std::ostringstream oss;
	oss << obs_module_text("Sequence #")
        << (seqId + 1)
        << obs_module_text(": at index #")
        << currMatchIdx
        << ", (" << matchLabel << ") "
        << (isActive ? obs_module_text("[active]")
                     : obs_module_text("[paused]"));

    m_statusDisplay->setText(oss.str().data());
}

void PmSequenceWidget::setSequenceId(int seqId)
{
    // todo: concurrency issues?
	m_sequenceId = seqId;
    onSequenceStateChanged(m_sequenceId);
}

void PmSequenceWidget::onStartResumeReleased()
{
    bool isActive = m_core->sequenceIsActive(m_sequenceId);
	if (isActive) {
	    emit sigSequencePause(m_sequenceId);
	} else {
		emit sigSequenceActivate(m_sequenceId);
    }
}

void PmSequenceWidget::onResetReleased()
{
	emit sigSequenceReset(m_sequenceId);
}

QPushButton *PmSequenceWidget::prepareButton(
    const char *tooltip, const char *icoPath, char *themeId)
{
	QIcon icon;
	icon.addFile(icoPath, QSize(), QIcon::Normal, QIcon::Off);

	QPushButton *ret = new QPushButton(icon, "", this);
	ret->setToolTip(tooltip);
	ret->setIcon(icon);
	ret->setIconSize(QSize(16, 16));
	ret->setMaximumSize(22, 22);
	ret->setFlat(true);
	ret->setProperty("themeID", QVariant(themeId));
	ret->setFocusPolicy(Qt::NoFocus);

	return ret;
}
