#include "pixel-match-core.hpp"
#include "pixel-match-dialog.hpp"

#include <obs-frontend-api.h>
#include <QAction>

PixelMatcher *pixelMatcherInstance;

void init_pixel_match_switcher()
{
    pixelMatcherInstance = new PixelMatcher();
}

void free_pixel_match_switcher()
{
    delete pixelMatcherInstance;
    pixelMatcherInstance = nullptr;
}

//------------------------------------

PixelMatcher::PixelMatcher()
: m_dialog(new PixelMatchDialog())
{
    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction("Pixel Match Switcher"));
    connect(action, &QAction::triggered, m_dialog, &QDialog::exec);
}



PixelMatcher *PixelMatcher::getInstance()
{
    return pixelMatcherInstance;
}
