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
}

QString PmProgressBar::text() const
{
    return QString("%1 - %2%%").arg(m_taskLabel).arg(value());
}

//===================================

PmRetrieverProgressDialog::PmRetrieverProgressDialog(
    PmPresetsRetriever *retriever, QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint
                | Qt::WindowCloseButtonHint)
, m_retriever(retriever)
{
	setWindowTitle(obs_module_text("Downloading Preset(s)"));

    // scroll area
    QWidget *scrollWidget = new QWidget(this);
    m_scrollLayout = new QVBoxLayout();
    scrollWidget->setLayout(m_scrollLayout);
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(scrollWidget);

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
        PmProgressBar *pb = new PmProgressBar(fileUrl.data(), this);
        m_scrollLayout->addWidget(pb);
        m_map[fileUrl] = pb;
    } else {
        PmProgressBar *pb = *find;
        int percent = int(float(dlNow) * 100.f / float(dlTotal));
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
        pb->setStyleSheet("QProgressBar {  background-color: green; };");
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


