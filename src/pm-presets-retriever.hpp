#pragma once

#include <QObject>

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

    void startDownload();
    //void halt();

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
    void downloadPresets(QList<QString> presetName);

signals:
    // xml download
	void sigXmlProgress(std::string xmlUrl, size_t dlNow, size_t dlTotal);
	void sigXmlFailed(std::string xmlUrl, QString error);
    void sigXmlPresetsAvailable(QList<std::string> presetNames);

protected slots:
    // xml download
	void onXmlDownload();
	void onXmlFailed(std::string xmlUrl, QString error);
	void onXmlSucceeded(std::string xmlUrl, QByteArray data);

    // images download
    //void onImageProgress(QString imageFilename, int percent);
    //void onImageFailed(QString imageFilename, QString error);

protected:

    std::string m_xmlUrl;
	PmMatchPresets m_presets;

    int m_numActiveDownloads = 0;
    QThread *m_thread;

    //PresetRetrieverState m_state = Idle;
};
