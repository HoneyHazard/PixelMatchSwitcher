#pragma once

#include <QDialog>

class PmAboutBox : public QDialog
{
    Q_OBJECT

public:
    PmAboutBox(QWidget* parent);

protected:
    static const char* k_aboutText;
};