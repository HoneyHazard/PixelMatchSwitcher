#include "pm-retriever-progress-dialog.hpp"

#include "pm-presets-retriever.hpp"

#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

//===============================

PmProgressBar::PmProgressBar(const QString &taskLabel, QWidget *parent)
: QProgressBar(parent)
, m_taskLabel(taskLabel)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAlignment(Qt::AlignCenter);
    setStyleSheet("QProgressBar {  background-color: darkBlue; };");
}

QString PmProgressBar::text() const
{
    return QString("%1 - %2%").arg(m_taskLabel).arg(value());
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
}

void PmRetrieverProgressDialog::onFileProgress(
    std::string fileUrl, size_t dlNow, size_t dlTotal)
{
    auto find = m_map.find(fileUrl);
    if (find == m_map.end()) {
        QString filename = QUrl(fileUrl.data()).fileName();
        PmProgressBar *pb = new PmProgressBar(filename, this);
        m_scrollLayout->addWidget(pb);
        const int maxSz = PmPresetsRetriever::k_numConcurrentDownloads * 2;
        if (m_map.size() < maxSz) {
            int height = pb->maximumHeight()
                + pb->contentsMargins().top() + pb->contentsMargins().bottom();
            m_scrollWidget->setMinimumHeight(m_map.size() * height);
	        //m_scrollArea->setFixedHeight(m_scrollWidget->minimumHeight());
        }
        
        m_map[fileUrl] = pb;
    } else {
        PmProgressBar *pb = *find;
        int percent = dlTotal ? int(float(dlNow) * 100.f / float(dlTotal))
                    : 0;
        pb->setValue(percent);
    }
}

void PmRetrieverProgressDialog::onFileFailed(std::string fileUrl, QString error)
{
    auto find = m_map.find(fileUrl);
    if (find != m_map.end()) {
        PmProgressBar *pb = *find;
        pb->setStyleSheet("QProgressBar {  background-color: red; };");
    }
}

void PmRetrieverProgressDialog::onFileSuccess(std::string fileUrl)
{
    auto find = m_map.find(fileUrl);
    if (find != m_map.end()) {
        PmProgressBar *pb = *find;
        pb->setStyleSheet("QProgressBar {  background-color: darkGreen; };");
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


