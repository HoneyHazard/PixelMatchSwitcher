#include "pixel-match-dialog.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-debug-tab.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-filter.h"

#include <obs-module.h>
#include <qt-display.hpp>
#include <display-helpers.hpp>

#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

#include <obs-module.h>

PmDialog::PmDialog(PmCore *pixelMatcher, QWidget *parent)
: QDialog(parent)
, m_core(pixelMatcher)
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));

    // main tab
    QFormLayout *mainTabLayout = new QFormLayout;
    mainTabLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // image path display and browse button
    QHBoxLayout *imgPathSubLayout = new QHBoxLayout;
    imgPathSubLayout->setContentsMargins(0, 0, 0, 0);

    m_imgPathEdit = new QLineEdit(this);
    m_imgPathEdit->setReadOnly(true);
    imgPathSubLayout->addWidget(m_imgPathEdit);

    QPushButton *browseImgPathBtn = new QPushButton(
        obs_module_text("Browse"), this);
    connect(browseImgPathBtn, &QPushButton::released,
            this, &PmDialog::onBrowseButtonReleased);
    imgPathSubLayout->addWidget(browseImgPathBtn);

    mainTabLayout->addRow(obs_module_text("Match Image: "), imgPathSubLayout);

    // color mode selection
    QHBoxLayout *colorSubLayout = new QHBoxLayout;
    colorSubLayout->setContentsMargins(0, 0, 0, 0);

    m_colorModeCombo = new QComboBox(this);
    m_colorModeCombo->insertItem(int(GreenMode), obs_module_text("Green"));
    m_colorModeCombo->insertItem(int(MagentaMode), obs_module_text("Pink"));
    m_colorModeCombo->insertItem(int(BlackMode), obs_module_text("Black"));
    m_colorModeCombo->insertItem(int(AlphaMode), obs_module_text("Alpha"));
    m_colorModeCombo->insertItem(int(CustomClrMode), obs_module_text("Custom"));
    connect(m_colorModeCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onColorComboIndexChanged()));
    colorSubLayout->addWidget(m_colorModeCombo);

    m_colorModeDisplay = new QLabel(this);
    m_colorModeDisplay->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorSubLayout->addWidget(m_colorModeDisplay);
    onColorComboIndexChanged();

    mainTabLayout->addRow(obs_module_text("Color Mode: "), colorSubLayout);

    // image/match display area
    m_filterDisplay = new OBSQTDisplay(this);
    m_filterDisplay->setSizePolicy(
        QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_filterDisplay->setMinimumSize(400, 300);
    mainTabLayout->addRow(m_filterDisplay);

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

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(m_filterDisplay->GetDisplay(),
                          PmDialog::drawPreview,
                          this);
    };
    connect(m_filterDisplay, &OBSQTDisplay::DisplayCreated,
            addDrawCallback);
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

void PmDialog::onBrowseButtonReleased()
{

}

void PmDialog::colorChanged(PmDialog::ColorMode mode, QColor color)
{
    QColor bgColor = color, textColor = Qt::black;
    const QString ss;

    switch(mode) {
    case GreenMode:
        color = QColor(0, 255, 0);
        textColor = Qt::white;
        break;
    case MagentaMode:
        color = QColor(255, 0, 255);
        textColor = Qt::white;
        break;
    case BlackMode:
        color = QColor(0, 0, 0);
        textColor = Qt::white;
        break;
    default:
        break;
    }

    m_colorModeDisplay->setText(color.name(QColor::HexArgb));
    m_colorModeDisplay->setStyleSheet(
        QString("background-color :%1; color: %2;")
            .arg(color.name(QColor::HexArgb))
            .arg(textColor.name(QColor::HexArgb)));
}

void PmDialog::onColorComboIndexChanged()
{
    // send color mode to core? obs data API?
    ColorMode mode = ColorMode(m_colorModeCombo->currentIndex());
    colorChanged(mode, QColor());
}
