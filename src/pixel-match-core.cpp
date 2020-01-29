#include "pixel-match-core.hpp"
#include "pixel-match-dialog.hpp"

void init_pixel_match_switcher()
{
    PixelMatcher::m_instance = new PixelMatcher();
}

void free_pixel_match_switcher()
{
    delete PixelMatcher::m_instance;
    PixelMatcher::m_instance.clear();
}

//------------------------------------

PixelMatcher::PixelMatcher()
: m_dialog(new PixelMatchDialog())
{

}
