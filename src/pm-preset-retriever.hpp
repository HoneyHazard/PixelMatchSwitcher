#pragma once

#include <QObject>

#include <curl/curl.h>
#include <string>
#include "pm-structs.hpp"

typedef int PmFileRetrieverProgressFunc(void *clientp,
    curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t ultotal, curl_off_t ulnow); 

typedef size_t PmFileRetrieverWriteFunc(
    void *ptr, size_t size, size_t nmemb, void *stream);

class PmFileRetrieverTask
{
public:
    static const int k_numRetries = 5;
    static const int k_msBeforeRetry = 3000;

    enum ImageRetrieverTask { Idle, Downloading, Halted, Done, Failed };

protected:
    QString m_url;

    int retriesLeft = k_numRetries;
    int msLeftBeforeRetry = k_msBeforeRetry;

    PmFileRetrieverProgressFunc m_progressFunc;
    PmFileRetrieverWriteFunc m_writeFunc;
    CURL *m_curlHandle = nullptr;
};

class PmPresetRetriever : public QObject
{
    Q_OBJECT

public:
    static const int k_numConcurrentDownloads = 3;
    enum PresetRetrieverState
        { Idle, DownloadingXML, MakeSelection, DownloadingImages };

    // TODO: halted?

public slots:
    void downloadXML(QString url);
    void retrievePresets(QList<std::string> presetNames);

signals:
    // xml download
    void xmlProgress(int percent);
    void xmlPresetsFailed(QString error);
    PmMatchPresets xmlPresetsAvailable();

    // images download
    void imageProgress(QString presetName, int matchIndex, int percent);
    void imagesFailed(QString error);

protected:
    PresetRetrieverState m_state = Idle;
    QThread *m_thread;
};
