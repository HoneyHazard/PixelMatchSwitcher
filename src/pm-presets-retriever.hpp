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
    PmFileRetriever(std::string fileUrl, QObject *parent = nullptr);
    ~PmFileRetriever();


    QFuture<CURLcode> startDownload();

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

class PmPresetsRetriever : public QObject
{
    Q_OBJECT

public:
    static const int k_numConcurrentDownloads = 3;

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
    void sigPresetsReady(PmMatchPresets presets);

protected slots:
    // xml download
    void onXmlDownload(std::string xmlUrl);
    void onXmlFailed(std::string xmlUrl, int curlCode);
    void onXmlSucceeded(std::string xmlUrl, QByteArray data);

    void onRetrievePresets(QList<std::string> selectedPresets);

protected:

    std::string m_xmlUrl;
    PmMatchPresets m_availablePresets;

    int m_numActiveDownloads = 0;
    QThread *m_thread;

    //enum PresetRetrieverState
    //    { Idle, DownloadingXML, MakeSelection, DownloadingImages };

    // images download
    //void onImageProgress(QString imageFilename, int percent);
    //void onImageFailed(QString imageFilename, QString error);
    //PresetRetrieverState m_state = Idle;
};
