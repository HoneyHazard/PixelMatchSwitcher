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
    enum State { Progress, Failed, Success };

    PmProgressBar(const QString &taskLabel, QWidget *parent);
    QString text() const override { return m_text; }
    void setProgress(size_t dlNow, size_t dlTotal);
    void setFailed(QString reason = "");
    void setSuccess();

    State state() const { return m_state; }

protected:
    QString m_text;
    QString m_taskName;
    State m_state = Progress;
};

/**
 * @brief Display progress of downloads and allows abort or retry
 */
class PmPresetsRetrievalDialog : public QDialog
{
    Q_OBJECT

public:
    PmPresetsRetrievalDialog(PmPresetsRetriever *retriever, QWidget *parent);

signals:
    void sigAbort();
	void sigRetry();

protected slots:
    void onFileProgress(std::string fileUrl, size_t dlNow, size_t dlTotal);
    void onFileFailed(std::string fileUrl, QString error);
    void onFileSuccess(std::string fileUrl);

    void onFailed();
    void onSuccess();

protected:
    PmPresetsRetriever *m_retriever;

    QHash<std::string, PmProgressBar*> m_map;
    QScrollArea *m_scrollArea;
    QVBoxLayout *m_scrollLayout;
    QWidget *m_scrollWidget;
    QPushButton *m_retryButton;
};
