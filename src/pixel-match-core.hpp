#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <QPointer>

class PixelMatchDialog;

class PixelMatcher : public QObject
{
    Q_OBJECT
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();

public:
    static QPointer<PixelMatcher> getInstance() { return m_instance; }

    PixelMatcher();

private:
    static QPointer<PixelMatcher> m_instance;

    QPointer<PixelMatchDialog> m_dialog;

};
