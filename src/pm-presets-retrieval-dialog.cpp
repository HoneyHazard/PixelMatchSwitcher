#include "pm-presets-retrieval-dialog.hpp"

#include "pm-presets-retriever.hpp"

#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "pm-presets-retrieval-dialog.hpp"

//===============================

PmProgressBar::PmProgressBar(const QString &m_taskName, QWidget *parent)
: QProgressBar(parent)
, m_taskName(m_taskName)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAlignment(Qt::AlignCenter);
    setProgress(0, 0);
}

void PmProgressBar::setProgress(size_t dlNow, size_t dlTotal)
{
    int value = dlTotal ? (int)dlNow : 0;
    int range = dlTotal ? (int)dlTotal : 100;
    int percent = float(value) * 100.f / float(range);

    setStyleSheet(QString("QProgressBar { text-align: left; color: %1 }")
        .arg(percent < 50 ? "black" : "white"));

    m_text = QString("  %1 - %2%").arg(m_taskName).arg(percent);
    setRange(0, range);
    setValue(value);
    m_state = Progress;
}

void PmProgressBar::setFailed(QString reason)
{
    setStyleSheet(
        "QProgressBar { text-align: left; } "
        "QProgressBar::chunk {  background-color: red; }");
    m_text = QString("  %1 - %2")
        .arg(m_taskName)
        .arg(reason.size() ? reason : obs_module_text("FAILED"));
    setRange(0, 100);
    setValue(100);
    m_state = Failed;
}

void PmProgressBar::setSuccess()
{
    setStyleSheet("QProgressBar { text-align: left; } "
                  "QProgressBar::chunk {  background-color: green; }; ");
    m_text = QString("  %1").arg(m_taskName);
    setRange(0, 100);
    setValue(100);
    m_state = Success;
}

//===================================

PmPresetsRetrievalDialog::PmPresetsRetrievalDialog(
    PmPresetsRetriever *retriever, QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint
                | Qt::WindowCloseButtonHint)
, m_retriever(retriever)
{
    setWindowTitle(obs_module_text("Downloading Preset(s) Data"));
	setMinimumWidth(250);

    // scroll area to be filled with progress bars
    m_scrollWidget = new QWidget(this);
    m_scrollArea = new QScrollArea(this);
    m_scrollLayout = new QVBoxLayout(m_scrollWidget);
    m_scrollLayout->setContentsMargins(0, 0, 0, 0);
    m_scrollLayout->setAlignment(Qt::AlignTop);
    m_scrollArea->setWidget(m_scrollWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignTop);

    // button controls
    m_retryButton
        = new QPushButton(obs_module_text("Retry"), this);
    connect(m_retryButton, &QPushButton::released,
            this, &PmPresetsRetrievalDialog::sigRetry);
    m_retryButton->setVisible(false);

    QPushButton *cancelButton
        = new QPushButton(obs_module_text("Cancel"), this);
    connect(cancelButton, &QPushButton::released,
            this, &PmPresetsRetrievalDialog::sigAbort);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_retryButton);
    buttonsLayout->addWidget(cancelButton);

    // top level layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_scrollArea);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    // connections: retriever -> this
    const Qt::ConnectionType qc = Qt::QueuedConnection;
    connect(m_retriever, &PmPresetsRetriever::sigXmlProgress,
            this, &PmPresetsRetrievalDialog::onFileProgress, qc);
    connect(m_retriever, &PmPresetsRetriever::sigXmlFailed,
            this, &PmPresetsRetrievalDialog::onFileFailed, qc);
    connect(m_retriever, &PmPresetsRetriever::sigXmlPresetsAvailable,
            this, &PmPresetsRetrievalDialog::onFileSuccess, qc);

    connect(m_retriever, &PmPresetsRetriever::sigImgProgress,
            this, &PmPresetsRetrievalDialog::onFileProgress, qc);
    connect(m_retriever, &PmPresetsRetriever::sigImgFailed,
            this, &PmPresetsRetrievalDialog::onFileFailed, qc);
    connect(m_retriever, &PmPresetsRetriever::sigImgSuccess,
            this, &PmPresetsRetrievalDialog::onFileSuccess, qc);

    connect(m_retriever, &PmPresetsRetriever::sigFailed,
            this, &PmPresetsRetrievalDialog::onFailed, qc);
    connect(m_retriever, &PmPresetsRetriever::sigPresetsReady,
            this, &PmPresetsRetrievalDialog::onSuccess, qc);

    // connections: this -> retriever
    connect(this, &PmPresetsRetrievalDialog::sigAbort,
            m_retriever, &PmPresetsRetriever::onAbort, Qt::DirectConnection);
    connect(this, &PmPresetsRetrievalDialog::sigRetry,
            m_retriever, &PmPresetsRetriever::onRetry, qc);

    show();

    #if 0
    onFileProgress("progress", 100, 199);
    onFileProgress("failed", 0, 0);
    onFileFailed("failed", "who knows");
    onFileProgress("success", 0, 0);
    onFileSuccess("success");
    #endif
}

void PmPresetsRetrievalDialog::onFileProgress(
    std::string fileUrl, size_t dlNow, size_t dlTotal)
{
	m_retryButton->setVisible(false);
    PmProgressBar *pb;
    auto find = m_map.find(fileUrl);
    if (find == m_map.end()) {
        QString filename = QUrl(fileUrl.data()).fileName();
        QString taskLabel = filename.size() ? filename : fileUrl.data();
        pb = new PmProgressBar(taskLabel, this);
        m_scrollLayout->addWidget(pb);

        m_map[fileUrl] = pb;

        int spacing = m_scrollLayout->spacing();
        int height = pb->sizeHint().height() + spacing;
        int heightSeveral = m_map.size() * height - spacing;
        int width = qMax(
            m_scrollWidget->minimumWidth(), pb->sizeHint().width());

        m_scrollWidget->setMinimumHeight(heightSeveral);
        m_scrollWidget->setMinimumWidth(width);

        const int maxSz = PmPresetsRetriever::k_numConcurrentDownloads * 2;
        if (m_map.size() < maxSz) {
            m_scrollArea->setMinimumHeight(heightSeveral + spacing);
            m_scrollArea->setMinimumWidth(width + spacing);
            resize(sizeHint());
        }
        m_scrollArea->ensureWidgetVisible(pb);
    } else {
        pb = *find;
        if (pb->state() == PmProgressBar::Failed) {
            // re-insert at the end
            m_scrollLayout->removeWidget(pb);
            m_scrollLayout->addWidget(pb);
            m_scrollArea->ensureWidgetVisible(pb);
        }
    }
    pb->setProgress(dlNow, dlTotal);
}

void PmPresetsRetrievalDialog::onFileFailed(std::string fileUrl, QString error)
{
    auto find = m_map.find(fileUrl);
    if (find != m_map.end()) {
        PmProgressBar *pb = *find;
        pb->setFailed(error);
    }

}

void PmPresetsRetrievalDialog::onFileSuccess(std::string fileUrl)
{
    auto find = m_map.find(fileUrl);
    if (find != m_map.end()) {
        PmProgressBar *pb = *find;
        pb->setSuccess();
    }
}

void PmPresetsRetrievalDialog::onSuccess()
{
    deleteLater();
}

void PmPresetsRetrievalDialog::onFailed()
{
	m_retryButton->setVisible(true);
}
