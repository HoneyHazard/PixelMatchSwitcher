#pragma once

#include <QDialog>
#include <QPointer>

#include "pixel-match-structs.hpp"

class PmCore;

class PmDialog : public QDialog
{
    Q_OBJECT

public:
    PmDialog(PmCore *pixelMatcher, QWidget *parent);

signals:
    void sigOpenImage(QString filename);
    void sigNewUiConfig(PmMatchConfigPacket);

protected:
    QPointer<PmCore> m_core;
};
