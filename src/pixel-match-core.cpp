#include "pixel-match-core.hpp"
#include "pixel-match-dialog.hpp"

#include <obs-frontend-api.h>
#include <QAction>
#include <QMainWindow>

void init_pixel_match_switcher()
{
    PixelMatcher::m_instance = new PixelMatcher();
}

void free_pixel_match_switcher()
{
    delete PixelMatcher::m_instance;
    PixelMatcher::m_instance = nullptr;
}

//------------------------------------

PixelMatcher* PixelMatcher::m_instance = nullptr;

PixelMatcher::PixelMatcher()
{
    auto mainWindow = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    m_dialog = new PixelMatchDialog(this, mainWindow);

    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction("Pixel Match Switcher"));
    connect(action, &QAction::triggered, m_dialog, &QDialog::exec);
}

PixelMatcher *PixelMatcher::getInstance()
{
    return m_instance;
}
