#include "pm-presets-retriever.hpp"

//#include <obs-app.hpp>
#include <util/platform.h>

#include <QThread>
#include <QXmlStreamReader>
#include <QtConcurrent/QtConcurrent>

#include <sstream>

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

QFuture<CURLcode> PmFileRetriever::startDownload()
{
    return QtConcurrent::run(this, &PmFileRetriever::onDownload);
}

CURLcode PmFileRetriever::onDownload()
{
    //if (m_state == Downloading || m_state == Done) return;

    reset();

    m_curlHandle = curl_easy_init();

    blog(LOG_DEBUG, "curl download = %s", m_fileUrl.data());

    curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_fileUrl.data());
    curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, true);
    curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFOFUNCTION, staticProgressFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFODATA, this);

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, staticWriteFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, this);

    //m_state = Downloading;
    CURLcode result = curl_easy_perform(m_curlHandle);
    blog(LOG_DEBUG, "file retriever: curl perform result = %d", result);
    if (result == CURLE_OK) {
        emit sigSucceeded(m_fileUrl, m_data);
    } else {
        std::string errorInfo = curl_easy_strerror(result);
        emit sigFailed(m_fileUrl, result);
    }
    return result;
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

    // use signals & slots to run things in its own thread
    connect(this, &PmPresetsRetriever::sigDownloadXml,
            this, &PmPresetsRetriever::onXmlDownload, Qt::QueuedConnection);
    connect(this, &PmPresetsRetriever::sigRetrievePresets,
            this, &PmPresetsRetriever::onRetrievePresets, Qt::QueuedConnection);

    // CURL global init
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    blog(LOG_DEBUG, "preset retriever: curl init result = %d", result);
}

void PmPresetsRetriever::downloadXml(std::string xmlUrl)
{
    emit sigDownloadXml(xmlUrl);
}

void PmPresetsRetriever::retrievePresets(QList<std::string> selectedPresets)
{
    emit sigRetrievePresets(selectedPresets);
}

void PmPresetsRetriever::onXmlDownload(std::string xmlUrl)
{
    // initiate xml downloader
    m_xmlUrl = xmlUrl;
    auto *xmlDownloader = new PmFileRetriever(m_xmlUrl, this);
    connect(xmlDownloader, &PmFileRetriever::sigProgress, this,
        &PmPresetsRetriever::sigXmlProgress);
    connect(xmlDownloader, &PmFileRetriever::sigFailed, this,
        &PmPresetsRetriever::onXmlFailed);
    connect(xmlDownloader, &PmFileRetriever::sigSucceeded, this,
        &PmPresetsRetriever::onXmlSucceeded);
    xmlDownloader->startDownload();
}

void PmPresetsRetriever::onXmlFailed(std::string xmlUrl, int curlCode)
{
    // TODO: retry?

    QString errorString = curl_easy_strerror((CURLcode)curlCode);

    emit sigXmlFailed(xmlUrl, errorString);
    deleteLater();
}

void PmPresetsRetriever::onXmlSucceeded(std::string xmlUrl, QByteArray xmlData)
{
    try {
        m_availablePresets = PmMatchPresets(xmlData);
        emit sigXmlPresetsAvailable(m_availablePresets.keys());
    } catch (std::exception e) {
        emit sigXmlFailed(xmlUrl, e.what());
        deleteLater();
    } catch (...) {
        emit sigXmlFailed(
            xmlUrl, obs_module_text("Unknown error while parsing presets xml"));
        deleteLater();
    }
}

void PmPresetsRetriever::onRetrievePresets(QList<std::string> selectedPresets)
{
    PmMatchPresets readyPresets;

    try {

        // hex suffix based on URL
        uint urlHash = qHash(m_xmlUrl);
        std::ostringstream oss;
        oss << std::hex << urlHash;
        std::string urlHashSuffix = oss.str();

        std::string storePath = os_get_config_path_ptr(
            "obs-studio/plugin_config/PixelMatchSwitcher/presets");

        for (const std::string &presetName : selectedPresets) {
            PmMultiMatchConfig mcfg = m_availablePresets[presetName];

            // unique folder name based on url and preset name
            std::string presetPath
                = storePath + presetName + '_' + urlHashSuffix;
            os_mkdirs(presetPath.data());

            for (PmMatchConfig& cfg : mcfg) {

            }
        }

    } catch (std::exception e) {

    } catch (...) {

    }
}

