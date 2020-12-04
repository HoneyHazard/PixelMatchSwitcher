#include <QList>
#include <QSet>
#include <QDialog>
#include <string>

class QCheckBox;

class PmListSelectorDialog : public QDialog
{
    Q_OBJECT

public:
    PmListSelectorDialog(
        QString title,
        const QList<std::string> &availablePresets,
        const QList<std::string> &selectedPresets,
        QWidget *parent = nullptr);
    QList<std::string> selectedChoices() const;

protected:
    void setAll(bool value);

    QVector<QCheckBox*> m_checkboxes;
    QList<std::string> m_availableChoices;
};
