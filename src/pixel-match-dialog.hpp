#pragma once

#include <QDialog>

class PixelMatcher;

class PixelMatchDialog : public QDialog
{
    Q_OBJECT

public:
    PixelMatchDialog(PixelMatcher *pixelMatcher, QWidget *parent);

private:

};
