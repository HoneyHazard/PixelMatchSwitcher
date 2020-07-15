#pragma once

#include <QGroupBox>

class PmCore;

class PmHelpWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmHelpWidget(PmCore* core, QWidget* parent);

protected slots:
    // local UI event handlers
    void onShowDebug();
    void onShowAbout();

protected:
    PmCore* m_core;
};