#pragma once

#include <QObject>

#include <curl/curl.h>
#include <string>
#include "pm-structs.hpp"

class PmFileRetriever : public QObject
{
    Q_OBJECT

public:
    PmFileRetriever(QString fileUrl, QObject *parent = nullptr);
    ~PmFileRetriever();

    void startDownload();

signals:
    void sigFailed(QString urlName, QString error);
    void sigSucceeded(QString urlName, QByteArray byteArray);
    void sigProgress(QString urlName, size_t dlNow, size_t dlTotal);

protected:
    static int staticProgressFunc(void *clientp,
        curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow);
    static size_t staticWriteFunc(
        void *ptr, size_t size, size_t nmemb, void *data);

    void reset();

    QString m_fileUrl;
    QByteArray m_data;

    CURL *m_curlHandle = nullptr;
};

class PmPresetsRetriever : public QObject
{
    Q_OBJECT

public:
    static const int k_numConcurrentDownloads = 3;

    PmPresetsRetriever(QString xmlUrl, QObject *parent = nullptr);

    void downloadPresets(QList<QString> presetName);

signals:
    void sigXmlProgress(QString xmlUrl, size_t dlNow, size_t dlTotal);
    void sigXmlFailed(QString xmlUrl, QString error);
    void sigXmlPresetsAvailable(QList<std::string> presetNames);

protected slots:
    void onXmlFailed(QString xmlUrl, QString error);
    void onXmlSucceeded(QString xmlUrl, QByteArray data);

protected:
    QString m_xmlUrl;
    PmMatchPresets m_presets;

    int m_numActiveDownloads = 0;
    QThread *m_thread;
};
