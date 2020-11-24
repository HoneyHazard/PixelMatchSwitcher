#include "pm-preset-retriever.hpp"

#include <QThread>
#include <QXmlStreamReader>

PmFileRetriever::PmFileRetriever(QString fileUrl, QObject* parent)
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

    curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_fileUrl);

    curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFOFUNCTION, staticProgressFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFODATA, this);

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, staticWriteFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, this);

    //m_state = Downloading;
    curl_easy_perform(m_curlHandle);
}

int PmFileRetriever::staticProgressFunc(
    void *clientp,
    curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t ultotal, curl_off_t ulnowCould )
{
    PmFileRetriever *r = (PmFileRetriever *)clientp;
    r->m_data.reserve(dltotal);

    emit r->sigProgress(r->m_fileUrl, dlnow, dltotal);

    return 0;
}

size_t PmFileRetriever::staticWriteFunc(
    void *ptr, size_t size, size_t nmemb, void *data)
{
    PmFileRetriever *r = (PmFileRetriever *)data;
    r->m_data.append((const char*)ptr, size);
    if (r->m_data.size() == r->m_data.capacity()) {
        emit r->sigSucceeded(r->m_fileUrl, r->m_data);
        r->deleteLater();
    }
    return size;
}

//========================================================

PmPresetsRetriever::PmPresetsRetriever(QString xmlUrl, QObject* parent)
: QObject(parent)
, m_xmlUrl(xmlUrl)
{
    curl_global_init(CURL_GLOBAL_ALL);

    // move to own thread
    m_thread = new QThread(this);
    m_thread->setObjectName("preset retriever thread");

    // initiate xml downloader
    auto *xmlDownloader = new PmFileRetriever(xmlUrl, this);
    connect(xmlDownloader, &PmFileRetriever::sigProgress,
            this, &PmPresetsRetriever::sigXmlProgress);
    connect(xmlDownloader, &PmFileRetriever::sigFailed,
            this, &PmPresetsRetriever::onXmlFailed);
    connect(xmlDownloader, &PmFileRetriever::sigSucceeded,
            this, &PmPresetsRetriever::onXmlSucceeded);
    xmlDownloader->startDownload();
}

void PmPresetsRetriever::onXmlFailed(QString xmlUrl, QString error)
{
    emit sigXmlFailed(xmlUrl, error);
    deleteLater();
}

void PmPresetsRetriever::onXmlSucceeded(QString xmlUrl, QByteArray xmlData)
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
