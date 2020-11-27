#pragma once

#include <QObject>
#include <QByteArray>
#include <QtConcurrent/QtConcurrent>

#include <curl/curl.h>
#include <string>

#include "pm-structs.hpp"

/**
 * @brief Wrapper class for downloads using libcurl
 */
class PmFileRetriever : public QObject
{
    Q_OBJECT

public:
    PmFileRetriever(std::string fileUrl, QObject *parent = nullptr);
    ~PmFileRetriever();

    QFuture<CURLcode> startDownload(QThreadPool& threadPool);

    void setSaveFilename(const std::string& saveFilename)
        { m_saveFilename = saveFilename; }

    QFuture<CURLcode>& future() { return m_future;}

    const QByteArray &data() const { return m_data; }

signals:
    void sigFailed(std::string urlName, int curlCode);
    void sigSucceeded(std::string urlName, QByteArray byteArray);
    void sigProgress(std::string urlName, size_t dlNow, size_t dlTotal);

protected slots:
    CURLcode onDownload();

protected:
    static int staticProgressFunc(void *clientp,
        curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow);
    static size_t staticWriteFunc(
        void *ptr, size_t size, size_t nmemb, void *data);

    void reset();

    std::string m_fileUrl;
    std::string m_saveFilename;
    QByteArray m_data;
    QFuture<CURLcode> m_future;

    CURL *m_curlHandle = nullptr;

    //static const int k_numRetries = 5;
    //static const int k_msBeforeRetry = 3000;

    //enum FileRetrieverState {
    //    Idle, Downloading, RetryPending, Halted, Done, Failed };
    //FileRetrieverState m_state = Idle;
    //FileRetrieverState state() const { return m_state; }
    //void halt();

    //int retriesLeft = k_numRetries;
    //int msLeftBeforeRetry = k_msBeforeRetry;
};

/**
 * @brief Downloads preset XML from URL and associated images; 
 *        presents local presets
 */
class PmPresetsRetriever : public QObject
{
    Q_OBJECT

public:
    static const int k_numConcurrentDownloads = 3;

    PmPresetsRetriever(std::string xmlUrl);

    void downloadXml();
    void retrievePresets(QList<std::string> selectedPresets);
    void retryImages();
    void abort();

signals:
    // forwarded to self to run in its own thread
    void sigRetrievePresets(QList<std::string> selectedPresets);

    // xml phase
    void sigXmlProgress(std::string xmlUrl, size_t dlNow, size_t dlTotal);
    void sigXmlFailed(std::string xmlUrl, QString error);
    void sigXmlPresetsAvailable(
        std::string xmlUrl, QList<std::string> presetNames);

    // image phase
    void sigImgProgress(std::string imgUrl, size_t dlNow, size_t dlTotal);
    void sigImgFailed(std::string imgUrl, QString error);
    void sigImgSuccess(std::string imgUrl);
    void sigPresetsReady(PmMatchPresets presets);

public slots:
    void onDownloadXml();
	void onRetrievePresets();
    void onDownloadImages();
    void onAbort();

protected slots:

    //void onXmlFailed(std::string xmlUrl, int curlCode);
    //void onXmlSucceeded(std::string xmlUrl, QByteArray data);

    // images download
    void onImgFailed(std::string imgUrl, int curlCode);

protected:
    std::string m_xmlUrl;
    PmMatchPresets m_availablePresets;
    QList<std::string> m_selectedPresets;

    bool m_abort = false;
    QThread *m_thread;
    QThreadPool m_workerThreadPool;
    QList<PmFileRetriever*> m_imgRetrievers;

    //enum PresetRetrieverState
    //    { Idle, DownloadingXML, MakeSelection, DownloadingImages };

    // images download
    //void onImageProgress(QString imageFilename, int percent);
    //void onImageFailed(QString imageFilename, QString error);
    //PresetRetrieverState m_state = Idle;
    //int m_numActiveDownloads = 0;
};
