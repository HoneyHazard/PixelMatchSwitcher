#include "pm-about-box.hpp"

#include <QTextBrowser>
#include <QVBoxLayout>

#include <obs-module.h>

const char* PmAboutBox::k_aboutText = obs_module_text(
    "<h2>About Pixel Match Switcher</h2>"
    "<p>Pixel Match Switcher plugin allows switching scenes based on contents"
    "   of video passing through a Pixel Match filter. </p>"
    "<p>Project's development home is at:<br />"
    "   <a href=\"https://github.com/HoneyHazard/PixelMatchSwitcher/\">"
    "          https://github.com/HoneyHazard/PixelMatchSwitcher</a></p>"
    "<p>The plugins presently requires a special fork of OBS to function:"
    "   <a href=\"https://github.com/HoneyHazard/obs-studio-atomic-effects\">"
    "    <br />https://github.com/HoneyHazard/obs-studio-atomic-effects</a></p>"
    "<h2>Attributions</h2>"
    "<p>In addition to being a plugin for OBS, Pixel Match Switcher also "
    "cannibalizes some icon resources and UI elements from OBS source code.</p>"
    "<p>Some of the icons used are derived from icons by ICON8:<br />"
    "<a href=\"https://icons8.com\">https://icons8.com</a></p>"
    "<h2>License</h2>"
    "<p><a href=\"https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html\">"
    "   GNU General Public License, version 2</a></p>"
);

PmAboutBox::PmAboutBox(QWidget* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint
                | Qt::WindowCloseButtonHint)
{
    setWindowTitle(obs_module_text("About"));

    QTextBrowser* tb = new QTextBrowser(this);
    tb->setOpenExternalLinks(true);
    tb->setReadOnly(true);
    tb->setText(k_aboutText);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(tb);
    setLayout(layout);

    resize(400, 400);
    exec();
}