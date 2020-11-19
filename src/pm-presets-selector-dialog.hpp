#include <QList>
#include <QSet>
#include <QDialog>
#include <string>

class QCheckBox;

class PmPresetsSelectorDialog : public QDialog
{
    Q_OBJECT

public:
    PmPresetsSelectorDialog(
        QString title,
        const QList<std::string> &availablePresets,
        const QList<std::string> &selectedPresets,
        QWidget *parent = nullptr);
    QList<std::string> selectedPresets() const;

protected slots:
    void onSelectAll();
    void onSelectNone();

protected:
    QVector<QCheckBox*> m_checkboxes;
    QList<std::string> m_availablePresets;
};
