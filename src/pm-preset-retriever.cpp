#include "pm-preset-retriever.hpp"

PmFileRetriever::PmFileRetriever(QString fileUrl, QObject* parent)
: QObject(parent)
{

}

void PmFileRetriever::startDownload()
{
    m_curlHandle = curl_easy_init();

    curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(m_curlHandle, CURLOPT_PROGRESSFUNCTION, staticProgressFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_PROGRESSDATA, this);

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, staticWriteFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, this);

}

int PmFileRetriever::staticProgressFunc(
    void *clientp,
    curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t ultotal, curl_off_t ulnow)
{
    return 0;
}

size_t PmFileRetriever::staticWriteFunc(
    void *ptr, size_t size, size_t nmemb, void *stream)
{
	return size_t();
}

PmPresetRetriever::PmPresetRetriever(QString xmlUrl, QObject* parent)
: QObject(parent)
{
    curl_global_init(CURL_GLOBAL_ALL);
}
