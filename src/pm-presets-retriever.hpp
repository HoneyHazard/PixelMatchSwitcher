#pragma once

#include <QObject>
#include <QtConcurrent/QtConcurrent>

#include <curl/curl.h>
#include <string>

#include "pm-structs.hpp"

class PmFileRetriever : public QObject
{
    Q_OBJECT

public:
    //static const int k_numRetries = 5;
    //static const int k_msBeforeRetry = 3000;

    //enum FileRetrieverState {
    //    Idle, Downloading, RetryPending, Halted, Done, Failed };

    PmFileRetriever(std::string fileUrl, QObject *parent = nullptr);
    ~PmFileRetriever();

    //FileRetrieverState state() const { return m_state; }

    QFuture<CURLcode> startDownload();
    //void halt();

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
    QByteArray m_data;

    //FileRetrieverState m_state = Idle;
    CURL *m_curlHandle = nullptr;

    //int retriesLeft = k_numRetries;
    //int msLeftBeforeRetry = k_msBeforeRetry;
};

class PmPresetsRetriever : public QObject
{
    Q_OBJECT

public:
    static const int k_numConcurrentDownloads = 3;
    //enum PresetRetrieverState
    //    { Idle, DownloadingXML, MakeSelection, DownloadingImages };

    PmPresetsRetriever(QObject *parent = nullptr);

    void downloadXml(std::string xmlFilename);
    void retrievePresets(QList<std::string> selectedPresets);

signals:
    // forwarded to self to run in its own thread
    void sigDownloadXml(std::string xmlUrl);
    void sigRetrievePresets(QList<std::string> selectedPresets);

    // xml download
    void sigXmlProgress(std::string xmlUrl, size_t dlNow, size_t dlTotal);
    void sigXmlFailed(std::string xmlUrl, QString error);
    void sigXmlPresetsAvailable(QList<std::string> presetNames);

    // finalize
    void sigXmlPresetsReady(PmMatchPresets presets);

protected slots:
    // xml download
    void onXmlDownload(std::string xmlUrl);
    void onXmlFailed(std::string xmlUrl, int curlCode);
    void onXmlSucceeded(std::string xmlUrl, QByteArray data);

    void onRetrievePresets(QList<std::string> selectedPresets);

    // images download
    //void onImageProgress(QString imageFilename, int percent);
    //void onImageFailed(QString imageFilename, QString error);

protected:

    std::string m_xmlUrl;
    PmMatchPresets m_availablePresets;

    int m_numActiveDownloads = 0;
    QThread *m_thread;

    //PresetRetrieverState m_state = Idle;
};
