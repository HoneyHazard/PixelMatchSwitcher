#include "pm-presets-retriever.hpp"

#include <QThread>
#include <QXmlStreamReader>
#include <QtConcurrent/QtConcurrent>

PmFileRetriever::PmFileRetriever(std::string fileUrl, QObject* parent)
: QObject(parent)
, m_fileUrl(fileUrl)
{

}

PmFileRetriever::~PmFileRetriever()
{
    reset();
}

void PmFileRetriever::reset()
{
    m_data.clear();
    if (m_curlHandle) {
        curl_easy_cleanup(m_curlHandle);
        m_curlHandle = nullptr;
    }
}

void PmFileRetriever::startDownload()
{
    //if (m_state == Downloading || m_state == Done) return;

    reset();

    m_curlHandle = curl_easy_init();

    blog(LOG_DEBUG, "curl download = %s", m_fileUrl.data());

    curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_fileUrl.data());
    curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
    //curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, true);
    //curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFOFUNCTION, staticProgressFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFODATA, this);

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, staticWriteFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, this);

    //m_state = Downloading;
    CURLcode result = curl_easy_perform(m_curlHandle);

    blog(LOG_DEBUG, "curl perform result = %d", result);
}

int PmFileRetriever::staticProgressFunc(
    void *clientp,
    curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t ultotal, curl_off_t ulnowCould)
{
    PmFileRetriever *r = (PmFileRetriever *)clientp;
    r->m_data.reserve(dltotal);

    emit r->sigProgress(r->m_fileUrl, dlnow, dltotal);

    return 0;

    UNUSED_PARAMETER(ultotal);
    UNUSED_PARAMETER(ulnowCould);
}

size_t PmFileRetriever::staticWriteFunc(
    void *ptr, size_t size, size_t nmemb, void *data)
{
    PmFileRetriever *r = (PmFileRetriever *)data;
	size_t realSize = size * nmemb;
    r->m_data.append((const char*)ptr, (int)realSize);
    if (r->m_data.size() == r->m_data.capacity()) {
        emit r->sigSucceeded(r->m_fileUrl, r->m_data);
        r->deleteLater();
    }
    return realSize;
}

//========================================================

PmPresetsRetriever::PmPresetsRetriever(QObject *parent)
: QObject(parent)
{
    // move to own thread
    m_thread = new QThread(this);
    m_thread->setObjectName("preset retriever thread");
    moveToThread(m_thread);

    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);

    blog(LOG_DEBUG, "curl init result = %d", result);
}

void PmPresetsRetriever::downloadXml(std::string xmlUrl)
{
    m_xmlUrl = xmlUrl;
    onXmlDownload();
}

void PmPresetsRetriever::onXmlDownload()
{
    // initiate xml downloader
    auto *xmlDownloader = new PmFileRetriever(m_xmlUrl, this);
    connect(xmlDownloader, &PmFileRetriever::sigProgress, this,
        &PmPresetsRetriever::sigXmlProgress);
    connect(xmlDownloader, &PmFileRetriever::sigFailed, this,
        &PmPresetsRetriever::onXmlFailed);
    connect(xmlDownloader, &PmFileRetriever::sigSucceeded, this,
        &PmPresetsRetriever::onXmlSucceeded);
    xmlDownloader->startDownload();
}

void PmPresetsRetriever::onXmlFailed(std::string xmlUrl, QString error)
{
    emit sigXmlFailed(xmlUrl, error);
    deleteLater();
}

void PmPresetsRetriever::onXmlSucceeded(std::string xmlUrl, QByteArray xmlData)
{
    try {
        m_presets = PmMatchPresets(xmlData);
        emit sigXmlPresetsAvailable(m_presets.keys());
    } catch (std::exception e) {
        onXmlFailed(xmlUrl, e.what());
    } catch (...) {
        onXmlFailed(
            xmlUrl, obs_module_text("Unknown error during xml import"));
    } 
}

