#include "pixel-match-matching-tab.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-filter.h"

#include <qt-display.hpp>
#include <display-helpers.hpp>

#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>

#include <obs-module.h>

PmMatchingTab::PmMatchingTab(PmCore *pixelMatcher, QWidget *parent)
: m_core(pixelMatcher)
{
    // init config
    auto config = m_core->config();

    // main tab
    QFormLayout *mainTabLayout = new QFormLayout;
    mainTabLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // image path display and browse button
    QHBoxLayout *imgPathSubLayout = new QHBoxLayout;
    imgPathSubLayout->setContentsMargins(0, 0, 0, 0);

    m_imgPathEdit = new QLineEdit(this);
    m_imgPathEdit->setReadOnly(true);
    m_imgPathEdit->setText(m_core->matchImgFilename());
    imgPathSubLayout->addWidget(m_imgPathEdit);

    QPushButton *browseImgPathBtn = new QPushButton(
        obs_module_text("Browse"), this);
    connect(browseImgPathBtn, &QPushButton::released,
            this, &PmMatchingTab::onBrowseButtonReleased);
    imgPathSubLayout->addWidget(browseImgPathBtn);

    mainTabLayout->addRow(obs_module_text("Image: "), imgPathSubLayout);

    // color mode selection
    QHBoxLayout *colorSubLayout = new QHBoxLayout;
    colorSubLayout->setContentsMargins(0, 0, 0, 0);

    m_maskModeCombo = new QComboBox(this);
    m_maskModeCombo->insertItem(
        int(PmMaskMode::GreenMode), obs_module_text("Green"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::MagentaMode), obs_module_text("Magenta"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::BlackMode), obs_module_text("Black"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::AlphaMode), obs_module_text("Alpha"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::CustomClrMode), obs_module_text("Custom"));
    m_maskModeCombo->setCurrentIndex(int(config.maskMode));
    connect(m_maskModeCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onColorComboIndexChanged()));
    colorSubLayout->addWidget(m_maskModeCombo);

    m_maskModeDisplay = new QLabel(this);
    m_maskModeDisplay->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorSubLayout->addWidget(m_maskModeDisplay);

    mainTabLayout->addRow(obs_module_text("Mask Mode: "), colorSubLayout);

    // match location
    QHBoxLayout *matchLocSubLayout = new QHBoxLayout;
    matchLocSubLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *posXLabel = new QLabel("X = ", this);
    matchLocSubLayout->addWidget(posXLabel);
    m_posXBox = new QSpinBox(this);
    m_posXBox->setSuffix(" px");
    m_posXBox->setMinimum(0);
    m_posXBox->setSingleStep(1);
    m_posXBox->setValue(config.roiLeft);
    connect(m_posXBox, SIGNAL(valueChanged(int)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    matchLocSubLayout->addWidget(m_posXBox);
    matchLocSubLayout->addItem(new QSpacerItem(10, 1));

    QLabel *posYLabel = new QLabel("Y = ", this);
    matchLocSubLayout->addWidget(posYLabel);
    m_posYBox = new QSpinBox(this);
    m_posYBox->setSuffix(" px");
    m_posYBox->setMinimum(0);
    m_posYBox->setSingleStep(1);
    m_posYBox->setValue(config.roiBottom);
    connect(m_posYBox, SIGNAL(valueChanged(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    matchLocSubLayout->addWidget(m_posYBox);

    mainTabLayout->addRow(obs_module_text("Location: "), matchLocSubLayout);

    // per pixel error tolerance
    m_perPixelErrorBox = new QDoubleSpinBox(this);
    m_perPixelErrorBox->setSuffix(" %");
    m_perPixelErrorBox->setRange(0.0, 100.0);
    m_perPixelErrorBox->setSingleStep(1.0);
    m_perPixelErrorBox->setDecimals(1);
    m_perPixelErrorBox->setValue(double(config.perPixelErrThresh));
    connect(m_perPixelErrorBox, SIGNAL(valueChanged(double)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);

    mainTabLayout->addRow(
        obs_module_text("Allowed Pixel Error: "), m_perPixelErrorBox);

    // total match error threshold
    m_totalMatchThreshBox = new QDoubleSpinBox(this);
    m_totalMatchThreshBox->setSuffix(" %");
    m_totalMatchThreshBox->setRange(1.0, 100.0);
    m_totalMatchThreshBox->setSingleStep(1.0);
    m_totalMatchThreshBox->setDecimals(1);
    m_totalMatchThreshBox->setValue(double(config.totalMatchThresh));
    connect(m_totalMatchThreshBox, SIGNAL(valueChanged(double)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);

    mainTabLayout->addRow(
        obs_module_text("Global Match Threshold: "), m_totalMatchThreshBox);

    // match result label
    m_matchResultDisplay = new QLabel("--", this);
    m_matchResultDisplay->setTextFormat(Qt::RichText);
    mainTabLayout->addRow(
        obs_module_text("Match Result: "), m_matchResultDisplay);

    // preview mode
    m_previewModeButtons = new QButtonGroup(this);
    m_previewModeButtons->setExclusive(true);
    connect(m_previewModeButtons,SIGNAL(buttonReleased(int)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);

    QHBoxLayout *previewModeLayout = new QHBoxLayout;
    previewModeLayout->setContentsMargins(0, 0, 0, 0);

    QRadioButton *videoModeRadio
        = new QRadioButton(obs_module_text("Video"), this);
    m_previewModeButtons->addButton(videoModeRadio, int(PmPreviewMode::Video));
    previewModeLayout->addWidget(videoModeRadio);

    QRadioButton *regionModeRadio
        = new QRadioButton(obs_module_text("Region"), this);
    m_previewModeButtons->addButton(regionModeRadio, int(PmPreviewMode::Region));
    previewModeLayout->addWidget(regionModeRadio);

    QRadioButton *matchImgRadio
        = new QRadioButton(obs_module_text("Match Image"), this);
    m_previewModeButtons->addButton(matchImgRadio, int(PmPreviewMode::MatchImage));
    previewModeLayout->addWidget(matchImgRadio);

    // divider line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainTabLayout->addRow(line);

    m_previewModeButtons->button(int(config.previewMode))->setChecked(true);
    mainTabLayout->addRow(
        obs_module_text("Preview Mode: "), previewModeLayout);

    // preview scales
    m_previewScaleStack = new QStackedWidget(this);
    m_previewScaleStack->setSizePolicy(
        QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_videoScaleCombo = new QComboBox(this);
    m_videoScaleCombo->addItem("100%", 1.f);
    m_videoScaleCombo->addItem("75%", 0.75f);
    m_videoScaleCombo->addItem("50%", 0.5f);
    m_videoScaleCombo->addItem("25%", 0.25f);
    m_videoScaleCombo->setCurrentIndex(
        m_videoScaleCombo->findData(config.previewVideoScale));
    connect(m_videoScaleCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    m_previewScaleStack->insertWidget(
        int(PmPreviewMode::Video), m_videoScaleCombo);

    m_regionScaleCombo = new QComboBox(this);
    m_regionScaleCombo->addItem("50%", 0.5f);
    m_regionScaleCombo->addItem("100%", 1.f);
    m_regionScaleCombo->addItem("150%", 1.5f);
    m_regionScaleCombo->addItem("200%", 2.f);
    m_regionScaleCombo->addItem("500%", 5.f);
    m_regionScaleCombo->addItem("1000%", 10.f);
    m_regionScaleCombo->setCurrentIndex(
        m_regionScaleCombo->findData(config.previewRegionScale));
    connect(m_regionScaleCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    m_previewScaleStack->insertWidget(
        int(PmPreviewMode::Region), m_regionScaleCombo);

    m_matchImgScaleCombo = new QComboBox(this);
    m_matchImgScaleCombo->addItem("50%", 0.5f);
    m_matchImgScaleCombo->addItem("100%", 1.f);
    m_matchImgScaleCombo->addItem("150%", 1.5f);
    m_matchImgScaleCombo->addItem("200%", 2.f);
    m_matchImgScaleCombo->addItem("500%", 5.f);
    m_matchImgScaleCombo->addItem("1000%", 10.f);
    m_matchImgScaleCombo->setCurrentIndex(
        m_matchImgScaleCombo->findData(config.previewMatchImageScale));
    connect(m_matchImgScaleCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    m_previewScaleStack->insertWidget(
        int(PmPreviewMode::MatchImage), m_matchImgScaleCombo);

    mainTabLayout->addRow(
        obs_module_text("Preview Scale: "), m_previewScaleStack);

    // image/match display area
    m_filterDisplay = new OBSQTDisplay(this);
    mainTabLayout->addRow(m_filterDisplay);

    QWidget *mainTab = new QWidget(this);
    mainTab->setLayout(mainTabLayout);

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(m_filterDisplay->GetDisplay(),
                          PmMatchingTab::drawPreview,
                          this);
    };
    connect(m_filterDisplay, &OBSQTDisplay::DisplayCreated,
            addDrawCallback);

    // signals & slots
    connect(m_core, &PmCore::sigImgSuccess,
            this, &PmMatchingTab::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
            this, &PmMatchingTab::onImgFailed, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewResults,
            this, &PmMatchingTab::onNewResults, Qt::QueuedConnection);

    connect(this, &PmMatchingTab::sigOpenImage,
            m_core, &PmCore::onOpenImage, Qt::QueuedConnection);
    connect(this, &PmMatchingTab::sigNewUiConfig,
            m_core, &PmCore::onNewUiConfig, Qt::QueuedConnection);
}

void PmMatchingTab:: drawPreview(void *data, uint32_t cx, uint32_t cy)
{
    auto dialog = static_cast<PmMatchingTab*>(data);
    auto core = dialog->m_core;

    if (!core) return;

    auto config = core->config();
    if (config.previewMode == PmPreviewMode::MatchImage) {
        dialog->drawMatchImage();
    } else {
        dialog->drawEffect();
    }

    UNUSED_PARAMETER(cx);
    UNUSED_PARAMETER(cy);
}

void PmMatchingTab::drawEffect()
{
    auto config = m_core->config();
    auto filterRef = m_core->activeFilterRef();
    auto renderSrc = filterRef.filter();

    if (!renderSrc) return;

    float orthoLeft, orthoBottom, orthoRight, orthoTop;
    int vpLeft, vpBottom, vpWidth, vpHeight;

    if (config.previewMode == PmPreviewMode::Video) {
        QSize videoSz = m_core->videoBaseSize();
        QSize previewSz = videoSz * double(config.previewVideoScale);

        orthoLeft = 0.f;
        orthoBottom = 0.f;
        orthoRight = videoSz.width();
        orthoTop = videoSz.height();

        float scale;
        GetScaleAndCenterPos(
                    videoSz.width(), videoSz.height(),
                    previewSz.width(), previewSz.height(),
                    vpLeft, vpBottom, scale);
        vpWidth = previewSz.width();
        vpHeight = previewSz.height();
    } else if (config.previewMode == PmPreviewMode::Region) {
        const auto &results = m_prevResults;
        orthoLeft = config.roiLeft;
        orthoBottom = config.roiBottom;
        orthoRight = config.roiLeft + results.matchImgWidth;
        orthoTop = config.roiBottom + results.matchImgHeight;

        float scale = config.previewRegionScale;
        //float scale = config.previewVideoScale;
        vpLeft = 0.f;
        vpBottom = 0.0f;
        vpWidth = int(results.matchImgWidth * scale);
        vpHeight = int(results.matchImgHeight * scale);
    }

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -100.0f, 100.0f);
    gs_set_viewport(vpLeft, vpBottom, vpWidth, vpHeight);

    auto filterData = filterRef.filterData();
    filterRef.lockData();
    filterData->preview_mode = true;
    filterData->show_border = (config.previewMode == PmPreviewMode::Video);
    filterRef.unlockData();

    obs_source_video_render(renderSrc);

    gs_projection_pop();
    gs_viewport_pop();
}

void PmMatchingTab::drawMatchImage()
{
    auto config = m_core->config();
    auto filterRef = m_core->activeFilterRef();
    auto filterData = filterRef.filterData();

    float previewScale = config.previewMatchImageScale;

    gs_effect *effect = m_core->drawMatchImageEffect();
    gs_eparam_t *param = gs_effect_get_param_by_name(effect, "image");
    gs_effect_set_texture(param, filterData->match_img_tex);

    while (gs_effect_loop(effect, "Draw")) {
        gs_matrix_push();
        gs_matrix_scale3f(previewScale, previewScale, previewScale);
        gs_draw_sprite(filterData->match_img_tex, 0, 0, 0);
        gs_matrix_pop();
    }
}

void PmMatchingTab::maskModeChanged(PmMaskMode mode, QColor color)
{
    QColor bgColor = color, textColor = Qt::black;
    const QString ss;

    switch(mode) {
    case PmMaskMode::GreenMode:
        color = QColor(0, 255, 0);
        textColor = Qt::white;
        break;
    case PmMaskMode::MagentaMode:
        color = QColor(255, 0, 255);
        textColor = Qt::white;
        break;
    case PmMaskMode::BlackMode:
        color = QColor(0, 0, 0);
        textColor = Qt::white;
        break;
    default:
        break;
    }

    m_maskModeDisplay->setText(color.name(QColor::HexArgb));
    m_maskModeDisplay->setStyleSheet(
        QString("background-color :%1; color: %2;")
            .arg(color.name(QColor::HexArgb))
            .arg(textColor.name(QColor::HexArgb)));
}

void PmMatchingTab::onColorComboIndexChanged()
{
    // send color mode to core? obs data API?
    PmMaskMode mode = PmMaskMode(m_maskModeCombo->currentIndex());
    maskModeChanged(mode, QColor());
    onConfigUiChanged();
}

void PmMatchingTab::onBrowseButtonReleased()
{
    static const QString filter
        = "PNG (*.png);; JPEG (*.jpg *.jpeg);; BMP (*.bmp);; All files (*.*)";

    QString curPath = QFileInfo(m_imgPathEdit->text()).absoluteDir().path();

    QString path = QFileDialog::getOpenFileName(
        this, obs_module_text("Open an image file"), curPath,
        filter);
    if (path.isEmpty())
        onImgFailed(path);
    else
        emit sigOpenImage(path);
}

void PmMatchingTab::onImgSuccess(QString filename)
{
    m_imgPathEdit->setText(filename);

    m_imgPathEdit->setStyleSheet("");
}

void PmMatchingTab::onImgFailed(QString filename)
{
    m_imgPathEdit->setText(
        filename.size() ? QString("[FAILED] %1").arg(filename) : "");
    m_imgPathEdit->setStyleSheet("text-color: red");
}

void PmMatchingTab::updateFilterDisplaySize(
    const PmConfigPacket &config, const PmResultsPacket &results)
{
    int cx, cy;
    if (config.previewMode == PmPreviewMode::Video) {
        float scale = config.previewVideoScale;
        cx = int(results.baseWidth * scale);
        cy = int(results.baseHeight * scale);
    } else if (config.previewMode == PmPreviewMode::Region) {
        //float scale = config.previewRegionScale;
        float scale = config.previewRegionScale;
        cx = int(results.matchImgWidth * scale);
        cy = int(results.matchImgHeight * scale);
    } else { // PmPreviewMode::MatchImage
        float scale = config.previewMatchImageScale;
        cx = int(results.matchImgWidth * scale);
        cy = int(results.matchImgHeight * scale);
    }

    if (m_filterDisplay->width() != cx) {
        m_filterDisplay->setFixedWidth(cx);
    }
    if (m_filterDisplay->height() != cy) {
        m_filterDisplay->setFixedHeight(cy);
    }
}

void PmMatchingTab::onNewResults(PmResultsPacket results)
{
    if (m_prevResults.baseWidth != results.baseWidth
     || m_prevResults.matchImgWidth != results.matchImgWidth) {
        m_posXBox->setMaximum(results.baseWidth - int(results.matchImgWidth));
    }
    if (m_prevResults.baseHeight != results.baseHeight
     || m_prevResults.matchImgHeight != results.matchImgHeight) {
        m_posYBox->setMaximum(results.baseHeight - int(results.matchImgHeight));
    }

    QString matchLabel = results.isMatched
        ? obs_module_text("<font color=\"DarkGreen\">[MATCHED]</font>")
        : obs_module_text("<font color=\"DarkRed\">[NO MATCH]</font>");

    QString resultStr = QString(
        obs_module_text("%1 %2 out of %3 pixels matched (%4 %)"))
            .arg(matchLabel)
            .arg(results.numMatched)
            .arg(results.numCompared)
            .arg(double(results.percentageMatched), 0, 'f', 1);
    m_matchResultDisplay->setText(resultStr);

    updateFilterDisplaySize(m_core->config(), results);
    m_prevResults = results;
}

void PmMatchingTab::onConfigUiChanged()
{
    PmConfigPacket config;
    config.roiLeft = m_posXBox->value();
    config.roiBottom = m_posYBox->value();
    config.perPixelErrThresh = float(m_perPixelErrorBox->value());
    config.totalMatchThresh = float(m_totalMatchThreshBox->value());
    config.maskMode = PmMaskMode(m_maskModeCombo->currentIndex());
    config.customColor = 0xffffffff;

    int previewModeIdx = m_previewModeButtons->checkedId();
    m_previewScaleStack->setCurrentIndex(previewModeIdx);
    auto scaleCombo = m_previewScaleStack->widget(previewModeIdx);
    if (scaleCombo) {
        m_previewScaleStack->setFixedSize(scaleCombo->sizeHint());
    }

    config.previewMode = PmPreviewMode(previewModeIdx);
    config.previewVideoScale
        = m_videoScaleCombo->currentData().toFloat();
    config.previewRegionScale
        = m_regionScaleCombo->currentData().toFloat();
    config.previewMatchImageScale
        = m_matchImgScaleCombo->currentData().toFloat();

    updateFilterDisplaySize(config, m_prevResults);
    emit sigNewUiConfig(config);
}
