#pragma once

#include <QObject>
#include <QByteArray>
#include <QThreadPool>

#include <curl/curl.h>
#include <string>

#include "pm-structs.hpp"

/**
 * @brief Wrapper class for downloads using libcurl
 */
class PmFileRetriever : public QObject, public QRunnable
{
    Q_OBJECT

public:
    PmFileRetriever(std::string fileUrl, QObject *parent);
    ~PmFileRetriever();

    void waitForFinished();
    void abort() { m_abortFlag = 1; }
    void run() override;
    bool isStarted() const { return m_isStarted; }
    bool isFinished() const { return m_isFinished; }

    void setSaveFilename(const std::string& saveFilename)
        { m_saveFilename = saveFilename; }

    CURLcode result() { return m_result; }

    const QByteArray &data() const { return m_data; }

signals:
    void sigFailed(std::string urlName, QString error);
    void sigSucceeded(std::string urlName, QByteArray byteArray);
    void sigProgress(std::string urlName, size_t dlNow, size_t dlTotal);

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
    CURLcode m_result;

    CURL *m_curlHandle = nullptr;
    int m_abortFlag = 0;
    bool m_isStarted = false;
    bool m_isFinished = false;
};

/**
 * @brief Downloads preset XML from URL and associated images; 
 *        presents local presets
 */
class PmPresetsRetriever : public QObject
{
    Q_OBJECT

public:
    static const int k_numWorkerThreads = 4;

    PmPresetsRetriever(std::string xmlUrl);
    ~PmPresetsRetriever();

    void downloadXml();
    void retrievePresets(QList<std::string> selectedPresets);
    void downloadImages();

signals:
    // xml phase
    void sigXmlProgress(std::string xmlUrl, size_t dlNow, size_t dlTotal);
    void sigXmlFailed(std::string xmlUrl, QString error);
    void sigXmlPresetsAvailable(
        std::string xmlUrl, QList<std::string> presetNames);

    // image phase and results
    void sigImgProgress(std::string imgUrl, size_t dlNow, size_t dlTotal);
    void sigImgFailed(std::string imgUrl, QString error);
    void sigImgSuccess(std::string imgUrl);
    void sigPresetsReady(PmMatchPresets presets);
    void sigFailed();
    void sigAborted();

    // internal connections
    void sigDownloadXml();
    void sigRetrievePresets();
    void sigDownloadImages();

public slots:
    void onAbort();
    void onRetry();

protected slots:
    void onDownloadXml();
    void onRetrievePresets();
    void onDownloadImages();

protected:
    std::string m_xmlUrl;
    PmMatchPresets m_availablePresets;
    QList<std::string> m_selectedPresets;

    bool m_abort = false;
    QThreadPool m_workerThreadPool;
    QThread *m_retrieverThread = nullptr;
    PmFileRetriever* m_xmlRetriever = nullptr;
    QList<PmFileRetriever*> m_imgRetrievers;
};
