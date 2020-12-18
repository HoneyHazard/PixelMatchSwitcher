#include "pm-about-box.hpp"

#include <QTextBrowser>
#include <QVBoxLayout>

#include <obs-module.h>

const char* PmAboutBox::k_aboutText = obs_module_text(
    "<h1>About Pixel Match Switcher</h1>"
    "<p>Pixel Match Switcher plugin allows switching scenes or toggling scene "
    "     item visibility based on contents of video passing through a Pixel "
    "     Match filter. </p>"
    "<p>Source code and installer releases are available at:<br />"
    "    <a href=\"https://github.com/HoneyHazard/PixelMatchSwitcher/\">"
    "           https://github.com/HoneyHazard/PixelMatchSwitcher</a></p>"
    "<p>The plugins presently requires a special fork of OBS to function:"
    "    <a href=\"https://github.com/HoneyHazard/obs-studio-atomic-effects\">"
    "    <br />https://github.com/HoneyHazard/obs-studio-atomic-effects</a></p>"

    "<h2>User Manual</h2>"
    "<p><a href="
    "\"https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual\">"
    "https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual</a></p>"

    "<h2>Special Thank You</h2>"
    "<p>Special Thank You goes out to <a href=\"https://github.com/exeldro\">"
    "    Exceldro</a> for letting obs-move-transition plugin be included with "
    "    our installer builds:</p>"
    "<p><a href="
    "    \"https://github.com/exeldro/obs-move-transition\">"
    "    https://github.com/exeldro/obs-move-transition</a></p>"

    "<h2>Attributions</h2>"
    "<p>In addition to being a plugin for OBS, Pixel Match Switcher also "
    "    cannibalizes some icon resources and UI elements from OBS source code."
    "    </p>"
    "<p>Some of the icons used are derived from icons by ICON8:<br />"
    "    <a href=\"https://icons8.com\">https://icons8.com</a></p>"
    "<p>SMMPTE Color Bars by Denelson83: <br /> <a href="
    "    \"https://commons.wikimedia.org/wiki/File:SMPTE_Color_Bars.svg\">"
    "    https://commons.wikimedia.org/wiki/File:SMPTE_Color_Bars.svg</a>"

    "<h2>License</h2>"
    "<p><a href=\"https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html\">"
    "    GNU General Public License, version 2</a></p>"
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

    resize(400, 600);
    exec();
}
