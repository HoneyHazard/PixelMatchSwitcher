#include "pm-presets-retriever.hpp"

//#include <obs-app.hpp>
#include <util/platform.h>

#include <QThread>
#include <QXmlStreamReader>
#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include <QApplication>

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
    PmFileRetriever *r = static_cast<PmFileRetriever *>(clientp);
    r->m_data.reserve(int(dltotal));

    emit r->sigProgress(r->m_fileUrl, size_t(dlnow), size_t(dltotal));

    return r->m_abortFlag;

    UNUSED_PARAMETER(ultotal);
    UNUSED_PARAMETER(ulnowCould);
}

size_t PmFileRetriever::staticWriteFunc(
    void *ptr, size_t size, size_t nmemb, void *data)
{
    PmFileRetriever *r = static_cast<PmFileRetriever *>(data);
    size_t realSize = size_t(size * nmemb);
    r->m_data.append(static_cast<const char*>(ptr), int(realSize));
    return realSize;
}

//========================================================

PmPresetsRetriever::PmPresetsRetriever(std::string xmlUrl)
: QObject(nullptr)
, m_xmlUrl(xmlUrl)
{
    // worker thread pool for downloads
    m_workerThreadPool.setMaxThreadCount(k_numWorkerThreads);

    // CURL global init
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    blog(LOG_DEBUG, "preset retriever: curl init result = %d", result);
}

PmPresetsRetriever::~PmPresetsRetriever()
{
}

void PmPresetsRetriever::downloadXml()
{
    QtConcurrent::run(
        &m_workerThreadPool, this, &PmPresetsRetriever::onDownloadXmlWorker);
}

void PmPresetsRetriever::onDownloadXmlWorker()
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
        if (m_availablePresets.size() <= 1) {
            retrievePresets(m_availablePresets.keys());
        } else {
            emit sigXmlPresetsAvailable(m_xmlUrl, m_availablePresets.keys());
        }
    } catch (const std::exception &e) {
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

void PmPresetsRetriever::retrievePresets(QList<std::string> selectedPresets)
{
    m_selectedPresets = selectedPresets;
    QtConcurrent::run(
        &m_workerThreadPool, this, &PmPresetsRetriever::onRetrievePresets);
}

void PmPresetsRetriever::onRetrievePresets()
{
    // hex suffix based on URL
    std::ostringstream oss;
    oss << std::hex << qHash(m_xmlUrl);
    std::string urlHashSuffix = oss.str();

    // where preset images will be saved
    std::string storePath = os_get_config_path_ptr(
        "obs-studio/plugin_config/PixelMatchSwitcher/presets/");

    for (const std::string &presetName : m_selectedPresets) {
        PmMultiMatchConfig &mcfg = m_availablePresets[presetName];

        QString presetNameQstr = presetName.data();
        presetNameQstr.replace(PmConstants::k_problemCharacterRegex, "_");
        std::string presetNameSafe = presetNameQstr.toUtf8().data();

        blog(LOG_DEBUG, "sanitized preset name = %s", presetNameSafe.data());

        // unique folder name based on url and preset name
        std::string presetStorePath
            = storePath + presetNameSafe + '_' + urlHashSuffix + '/';

        blog(LOG_DEBUG, "presetStorePath = %s", presetStorePath.data());
        os_mkdirs(presetStorePath.data());

        for (PmMatchConfig& cfg : mcfg) {
            // filename
            std::string imgUrl = cfg.matchImgFilename;
            std::string imgFilename = QUrl(imgUrl.data())
                .fileName().toUtf8().data();
            std::string storeImgPath = presetStorePath + imgFilename;
            cfg.matchImgFilename = storeImgPath;
            cfg.wasDownloaded = true;

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

void PmPresetsRetriever::downloadImages()
{
    QtConcurrent::run(
        &m_workerThreadPool, this, &PmPresetsRetriever::onDownloadImages);
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
        for (const std::string &presetName : m_selectedPresets) {
            readyPresets[presetName] = m_availablePresets[presetName];
        }
        emit sigPresetsReady(readyPresets);
        deleteLater();
    } else {
        emit sigFailed();
    }
}

void PmPresetsRetriever::onAbort()
{
    if (m_xmlRetriever) {
        m_xmlRetriever->abort();
    }
    for (PmFileRetriever *imgRetriever : m_imgRetrievers) {
        imgRetriever->abort();
    }
    emit sigAborted();
    deleteLater();
}

void PmPresetsRetriever::onRetry()
{
    if (m_imgRetrievers.size()) {
        downloadImages();
    } else {
        downloadXml();
    }
}
