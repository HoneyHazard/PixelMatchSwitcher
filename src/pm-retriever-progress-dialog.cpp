#include "pm-retriever-progress-dialog.hpp"

#include "pm-presets-retriever.hpp"

#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

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

PmRetrieverProgressDialog::PmRetrieverProgressDialog(
    PmPresetsRetriever *retriever, QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint
                | Qt::WindowCloseButtonHint)
, m_retriever(retriever)
{
    setWindowTitle(obs_module_text("Downloading Preset(s) Data"));

    // scroll area to be filled with progress bars
    m_scrollWidget = new QWidget(this);
    m_scrollArea = new QScrollArea(this);
    m_scrollLayout = new QVBoxLayout(m_scrollWidget);
    m_scrollLayout->setContentsMargins(0, 0, 0, 0);
    m_scrollLayout->setAlignment(Qt::AlignTop);
    m_scrollArea->setWidget(m_scrollWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignTop);

    // button controls [wip]
    QPushButton *retryButton
        = new QPushButton(obs_module_text("Retry"), this);
    QPushButton *cancelButton
        = new QPushButton(obs_module_text("Cancel"), this);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(retryButton);
    buttonsLayout->addWidget(cancelButton);

    // top level layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_scrollArea);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    // connections
    const Qt::ConnectionType qc = Qt::QueuedConnection;
    connect(m_retriever, &PmPresetsRetriever::sigXmlProgress,
            this, &PmRetrieverProgressDialog::onFileProgress, qc);
    connect(m_retriever, &PmPresetsRetriever::sigXmlFailed,
            this, &PmRetrieverProgressDialog::onXmlFailed, qc);
    connect(m_retriever, &PmPresetsRetriever::sigXmlPresetsAvailable,
            this, &PmRetrieverProgressDialog::onXmlSuccess, qc);

    connect(m_retriever, &PmPresetsRetriever::sigImgProgress,
            this, &PmRetrieverProgressDialog::onFileProgress, qc);
    connect(m_retriever, &PmPresetsRetriever::sigImgFailed,
            this, &PmRetrieverProgressDialog::onImgFailed, qc);
    connect(m_retriever, &PmPresetsRetriever::sigImgSuccess,
            this, &PmRetrieverProgressDialog::onImgSuccess, qc);

    show();

    onFileProgress("test", 100, 199);
    onFileProgress("failed", 0, 0);
    onFileFailed("failed", "who knows");
}

void PmRetrieverProgressDialog::onFileProgress(
    std::string fileUrl, size_t dlNow, size_t dlTotal)
{
    PmProgressBar *pb;
    auto find = m_map.find(fileUrl);
    if (find == m_map.end()) {
        //QString filename = QUrl(fileUrl.data()).fileName();
        //QString taskLabel = filename.size() ? filename : fileUrl.data();
        QString taskLabel = fileUrl.data();
        pb = new PmProgressBar(taskLabel, this);
        m_scrollLayout->addWidget(pb);       

        m_map[fileUrl] = pb;

        int spacing = m_scrollLayout->spacing();
        int height = pb->sizeHint().height() + spacing;
        int heightSeveral = m_map.size() * height - spacing;
        m_scrollWidget->setMinimumHeight(heightSeveral);

        const int maxSz = PmPresetsRetriever::k_numConcurrentDownloads * 2;
        if (m_map.size() < maxSz) {
            m_scrollArea->setMinimumHeight(heightSeveral + spacing);
            resize(sizeHint());
        }
    } else {
        pb = *find;
    }
    pb->setProgress(dlNow, dlTotal);
}

void PmRetrieverProgressDialog::onFileFailed(std::string fileUrl, QString error)
{
    auto find = m_map.find(fileUrl);
    PmProgressBar *pb;
    if (find != m_map.end()) {
        PmProgressBar *pb = *find;
        pb->setFailed(error);
    }

}

void PmRetrieverProgressDialog::onFileSuccess(std::string fileUrl)
{
    auto find = m_map.find(fileUrl);
    if (find != m_map.end()) {
        PmProgressBar *pb = *find;
        pb->setSuccess();
    }
}

void PmRetrieverProgressDialog::onXmlFailed(
    std::string xmlUrl, QString error)
{
    onFileFailed(xmlUrl, error);
}

void PmRetrieverProgressDialog::onXmlSuccess(std::string xmlUrl)
{
    onFileSuccess(xmlUrl);
}

void PmRetrieverProgressDialog::onImgFailed(std::string imgUrl, QString error)
{
    onFileFailed(imgUrl, error);
}

void PmRetrieverProgressDialog::onImgSuccess(std::string imgUrl)
{
    onFileSuccess(imgUrl);
}


