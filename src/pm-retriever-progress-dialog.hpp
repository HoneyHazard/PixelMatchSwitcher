#pragma once

#include <QDialog>
#include <QProgressBar>
#include <QHash>

class PmPresetsRetriever;

class QVBoxLayout;
class QScrollArea;

/**
 * @brief Progress bar with custom text
 */
class PmProgressBar : public QProgressBar
{
    Q_OBJECT

public:
    PmProgressBar(const QString &taskLabel, QWidget *parent);
    QString text() const override;

protected:
    QString m_taskLabel;
};

/**
 * @brief Display progress of downloads and allows abort or retry
 */
class PmRetrieverProgressDialog : public QDialog
{
    Q_OBJECT

public:
    PmRetrieverProgressDialog(PmPresetsRetriever *retriever, QWidget *parent);

signals:
    void sigAbort();

protected slots:
    void onFileProgress(std::string fileUrl, size_t dlNow, size_t dlTotal);
    void onFileFailed(std::string fileUrl, QString error);
    void onFileSuccess(std::string fileUrl);

    void onXmlFailed(std::string xmlUrl, QString error);
    void onXmlSuccess(std::string xmlUrl);

    void onImgFailed(std::string imgUrl, QString error);
    void onImgSuccess(std::string imgUrl);

protected:
    PmPresetsRetriever *m_retriever;

    QHash<std::string, PmProgressBar*> m_map;
    QScrollArea *m_scrollArea;
    QVBoxLayout *m_scrollLayout;
    QWidget *m_scrollWidget;
};
