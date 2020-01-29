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
    static PixelMatcher* getInstance();

    PixelMatcher();

private:
    static PixelMatcher *m_instance;
    QPointer<PixelMatchDialog> m_dialog;

};
