#include "pixel-match-dialog.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-debug-tab.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-filter.h"

#include <obs-module.h>
#include <qt-display.hpp>
#include <display-helpers.hpp>

#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>

PmDialog::PmDialog(PmCore *pixelMatcher, QWidget *parent)
: QDialog(parent)
, m_core(pixelMatcher)
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));

    // main tab
    QGridLayout *mainTabLayout = new QGridLayout;

    //m_testLabel = new QLabel(this);
    //mainTabLayout->addWidget(m_testLabel, 0, 0);

    m_filterDisplay = new OBSQTDisplay(this);
    mainTabLayout->addWidget(m_filterDisplay, 0, 0);

    QWidget *mainTab = new QWidget(this);
    mainTab->setLayout(mainTabLayout);

    // debug tab
    PmDebugTab *debugTab = new PmDebugTab(pixelMatcher, this);

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(mainTab, obs_module_text("Main"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(tabWidget);
    setLayout(topLevelLayout);

    // connections
    connect(m_core, &PmCore::newFrameImage,
            this, &PmDialog::onNewFrameImage);

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(m_filterDisplay->GetDisplay(),
                          PmDialog::drawPreview,
                          this);
    };
    connect(m_filterDisplay, &OBSQTDisplay::DisplayCreated,
        addDrawCallback);
}

void PmDialog::onNewFrameImage()
{
    //auto pixMap = QPixmap::fromImage(m_pixelMatcher->frameImage());
    //m_testLabel->setPixmap(pixMap);
}

void PmDialog::drawPreview(void *data, uint32_t cx, uint32_t cy)
{
    auto dialog = static_cast<PmDialog*>(data);
    auto filterRef = dialog->m_core->activeFilterRef();
    auto renderSrc = filterRef.filter();
    if (!renderSrc) return;

    int sourceCX = int(filterRef.filterSrcWidth());
    int sourceCY = int(filterRef.filterSrcHeight());

    int x, y;
    int newCX, newCY;
    float scale;

    GetScaleAndCenterPos(sourceCX, sourceCY, int(cx), int(cy), x, y, scale);

    newCX = int(scale * float(sourceCX));
    newCY = int(scale * float(sourceCY));

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
    gs_set_viewport(x, y, newCX, newCY);

    obs_source_video_render(renderSrc);

    gs_projection_pop();
    gs_viewport_pop();
}
