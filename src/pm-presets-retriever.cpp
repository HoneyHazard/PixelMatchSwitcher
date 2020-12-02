#include "pm-presets-retriever.hpp"

//#include <obs-app.hpp>
#include <util/platform.h>

#include <QThread>
#include <QXmlStreamReader>
#include <QtConcurrent/QtConcurrent>
#include <QFile>

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

QFuture<CURLcode> PmFileRetriever::startDownload(QThreadPool &threadPool)
{
    m_future
        = QtConcurrent::run(&threadPool, this, &PmFileRetriever::onDownload);
    return m_future;
}

CURLcode PmFileRetriever::onDownload()
{
    //if (m_state == Downloading || m_state == Done) return;

    emit sigProgress(m_fileUrl, 0, 0);

    reset();

    m_curlHandle = curl_easy_init();

    blog(LOG_DEBUG, "curl download = %s", m_fileUrl.data());

    curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_fileUrl.data());
    curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, true);
    curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFOFUNCTION, staticProgressFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFODATA, this);

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, staticWriteFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, this);

    CURLcode result = curl_easy_perform(m_curlHandle);
    blog(LOG_DEBUG, "file retriever: curl perform result = %d", result);

    if (result == CURLE_OK) {
        if (m_saveFilename.size()) {
            QFile file(m_saveFilename.data());
            file.open(QIODevice::WriteOnly);
            if (!file.isOpen()) {
                emit sigFailed(m_fileUrl, file.errorString());
                return CURLE_WRITE_ERROR;
            }
            file.write(m_data);
            if (file.error()) {
                emit sigFailed(m_fileUrl, file.errorString());
                return CURLE_WRITE_ERROR;
            }
        }
        emit sigSucceeded(m_fileUrl, m_data);
    } else {
        QString errorInfo = curl_easy_strerror(result);
        emit sigFailed(m_fileUrl, errorInfo);
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

PmPresetsRetriever::PmPresetsRetriever(std::string xmlUrl)
: QObject(nullptr)
, m_xmlUrl(xmlUrl)
{
    // move to own thread
    m_thread = new QThread();
    m_thread->setObjectName("preset retriever thread");
    moveToThread(m_thread);
    m_thread->start();

    // worker thread pool for downloads
    m_workerThreadPool.setMaxThreadCount(k_numConcurrentDownloads);

    // CURL global init
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    blog(LOG_DEBUG, "preset retriever: curl init result = %d", result);
}

PmPresetsRetriever::~PmPresetsRetriever()
{
    m_thread->exit();
    while (m_thread->isRunning()) {
        QThread::msleep(10);
    }
}

void PmPresetsRetriever::downloadXml()
{
    QMetaObject::invokeMethod(
        this, &PmPresetsRetriever::onDownloadXml, Qt::QueuedConnection);
}

void PmPresetsRetriever::retrievePresets(QList<std::string> selectedPresets)
{
    m_selectedPresets = selectedPresets;
    QMetaObject::invokeMethod(
        this, &PmPresetsRetriever::onRetrievePresets, Qt::QueuedConnection);
}

void PmPresetsRetriever::retryImages()
{
    QMetaObject::invokeMethod(
        this, &PmPresetsRetriever::onDownloadImages, Qt::QueuedConnection);
}

void PmPresetsRetriever::abort()
{
    //QMetaObject::invokeMethod(
    //    this, &PmPresetsRetriever::onAbort, Qt::QueuedConnection);
	onAbort();
}

void PmPresetsRetriever::onDownloadXml()
{
    // initiate xml downloader
    m_xmlRetriever = new PmFileRetriever(m_xmlUrl, this);
    connect(m_xmlRetriever, &PmFileRetriever::sigProgress,
            this, &PmPresetsRetriever::sigXmlProgress, Qt::DirectConnection);

    // fetch the xml
    QFuture<CURLcode> xmlFuture
        = m_xmlRetriever->startDownload(m_workerThreadPool);
    xmlFuture.waitForFinished();
    CURLcode xmlResultCode = xmlFuture.result();
    if (xmlResultCode != CURLE_OK) {
        QString errorString = curl_easy_strerror(xmlResultCode);
        emit sigXmlFailed(m_xmlUrl, errorString);
        emit sigFailed();
        return;
    }

    try {
        // parse the xml data; report available presets
        const QByteArray &xmlData = m_xmlRetriever->data();
        m_availablePresets = PmMatchPresets(xmlData);
        emit sigXmlPresetsAvailable(m_xmlUrl, m_availablePresets.keys());
    } catch (std::exception e) {
        m_xmlRetriever->deleteLater();
	    m_xmlRetriever = nullptr;
        emit sigXmlFailed(m_xmlUrl, e.what());
        emit sigFailed();
    } catch (...) {
        m_xmlRetriever->deleteLater();
	    m_xmlRetriever = nullptr;
        emit sigXmlFailed(
            m_xmlUrl,
            obs_module_text("Unknown error while parsing presets xml"));
        emit sigFailed();
    }
}

void PmPresetsRetriever::onRetrievePresets()
{
    // hex suffix based on URL
    uint urlHash = qHash(m_xmlUrl);
    std::ostringstream oss;
    oss << std::hex << urlHash;
    std::string urlHashSuffix = oss.str();

    std::string storePath = os_get_config_path_ptr(
        "obs-studio/plugin_config/PixelMatchSwitcher/presets/");

    for (const std::string &presetName : m_selectedPresets) {
        PmMultiMatchConfig &mcfg = m_availablePresets[presetName];

        // unique folder name based on url and preset name
        std::string presetStorePath
            = storePath + presetName + '_' + urlHashSuffix + '/';
        os_mkdirs(presetStorePath.data());

        for (PmMatchConfig& cfg : mcfg) {
            // filename
            std::string imgUrl = cfg.matchImgFilename;
            std::string imgFilename = QUrl(imgUrl.data())
                .fileName().toUtf8().data();
            std::string storeImgPath = presetStorePath + imgFilename;
            cfg.matchImgFilename = storeImgPath;

            // prepare an image retriever
            const Qt::ConnectionType dc = Qt::DirectConnection;
            PmFileRetriever *imgRetriever
                = new PmFileRetriever(imgUrl, this);
            imgRetriever->setSaveFilename(storeImgPath);
            connect(imgRetriever, &PmFileRetriever::sigProgress,
                    this, &PmPresetsRetriever::sigImgProgress, dc);
            connect(imgRetriever, &PmFileRetriever::sigFailed,
                    this, &PmPresetsRetriever::sigImgFailed, dc);
            connect(imgRetriever, &PmFileRetriever::sigSucceeded,
                    this, &PmPresetsRetriever::sigImgSuccess, dc);
            m_imgRetrievers.push_back(imgRetriever);
        }
    }

    onDownloadImages();
}

void PmPresetsRetriever::onDownloadImages()
{
    // dispatch image retrievers needed to start or resume a failed retrieve
    for (PmFileRetriever *imgRetriever : m_imgRetrievers) {
        QFuture<CURLcode> &future = imgRetriever->future();
        if (!future.isStarted()
         || !future.isResultReadyAt(0)
         || future.result() != CURLE_OK) {
            imgRetriever->startDownload(m_workerThreadPool);
        }
    }

    // wait for completion; check for success
    bool imagesOk = true;
    for (PmFileRetriever *imgRetriever : m_imgRetrievers) {
        QFuture<CURLcode> &future = imgRetriever->future();
        future.waitForFinished();
        if (future.result() != CURLE_OK) {
            imagesOk = false;
        }
    }

    if (imagesOk) {
        // finalize the output
        PmMatchPresets readyPresets;
        for (const std::string presetName : m_selectedPresets) {
            readyPresets[presetName] = m_availablePresets[presetName];
        }
        emit sigPresetsReady(readyPresets);
    } else {
        emit sigFailed();
    }
}

void PmPresetsRetriever::onAbort()
{
    m_xmlRetriever->future().cancel();
    for (PmFileRetriever *imgRetriever : m_imgRetrievers) {
        imgRetriever->future().cancel();
    }

    m_xmlRetriever->future().waitForFinished();
    for (PmFileRetriever *imgRetriever : m_imgRetrievers) {
        imgRetriever->future().cancel();
    }

    m_thread->exit();
    deleteLater();
}

void PmPresetsRetriever::onRetry()
{
    if (m_imgRetrievers.size()) {
        onDownloadImages();
    } else {
        onDownloadXml();
    }
}

#if 0
void PmPresetsRetriever::onXmlFailed(std::string xmlUrl, int curlCode)
{
    QString errorString = curl_easy_strerror(xmlResultCode);
    emit sigXmlFailed(xmlUrl, errorString);
    deleteLater();
     TODO: allow retry?
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
#endif
