#include "pm-preset-retriever.hpp"

PmFileRetriever::PmFileRetriever(QString fileUrl, QObject* parent)
: QObject(parent)
{

}

void PmFileRetriever::startDownload()
{
    m_data.clear();

    m_curlHandle = curl_easy_init();

    curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_fileUrl);

    curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFOFUNCTION, staticProgressFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_XFERINFODATA, this);

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, staticWriteFunc);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, this);

    curl_easy_perform(m_curlHandle);
}

int PmFileRetriever::staticProgressFunc(
    void *clientp,
    curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t ultotal, curl_off_t ulnow)
{
	PmFileRetriever *r = (PmFileRetriever *)clientp;
	r->m_data.reserve(dltotal);

    return 0;
}

size_t PmFileRetriever::staticWriteFunc(
    void *ptr, size_t size, size_t nmemb, void *data)
{
    PmFileRetriever *r = (PmFileRetriever *)data;
    r->m_data.append((const char*)ptr, size);
    return size;
}

PmPresetRetriever::PmPresetRetriever(QString xmlUrl, QObject* parent)
: QObject(parent)
{
    curl_global_init(CURL_GLOBAL_ALL);
}
