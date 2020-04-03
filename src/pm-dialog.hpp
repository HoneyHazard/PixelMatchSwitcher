#pragma once

#include <QDialog>
#include <QPointer>

#include "pm-structs.hpp"

class PmCore;

class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);

signals:
    void sigOpenImage(QString filename);
    void sigNewUiConfig(PmMatchConfig);

protected:
    void closeEvent(QCloseEvent*) override;

    QPointer<PmCore> m_core;
};
