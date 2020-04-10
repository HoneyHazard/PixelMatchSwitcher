#include "pm-match-tab.hpp"
#include "pm-core.hpp"
#include "pm-filter.h"

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
#include <QInputDialog>
#include <QMessageBox>
#include <QThread> // sleep

#include <obs-module.h>

const char * PmMatchTab::k_unsavedPresetStr
    = obs_module_text("<unsaved preset>");

PmMatchTab::PmMatchTab(PmCore *pixelMatcher, QWidget *parent)
: QWidget(parent)
, m_core(pixelMatcher)
{
    // main layout
    QFormLayout *mainLayout = new QFormLayout;
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // preset controls
    QHBoxLayout *presetLayout = new QHBoxLayout;
    presetLayout->setContentsMargins(0, 0, 0, 0);

    m_presetCombo = new QComboBox(this);
    m_presetCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    m_presetCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(m_presetCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onPresetSelected()));
    presetLayout->addWidget(m_presetCombo);

    m_presetSaveButton = new QPushButton(obs_module_text("Save"), this);
    connect(m_presetSaveButton, &QPushButton::released,
            this, &PmMatchTab::onPresetSave, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetSaveButton);

    m_presetSaveAsButton = new QPushButton(obs_module_text("Save As"), this);
    connect(m_presetSaveAsButton, &QPushButton::released,
            this, &PmMatchTab::onPresetSaveAs, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetSaveAsButton);

    m_presetResetButton = new QPushButton(obs_module_text("Reset"), this);
    connect(m_presetResetButton, &QPushButton::released,
            this, &PmMatchTab::onConfigReset, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetResetButton);

    m_presetRemoveButton = new  QPushButton(obs_module_text("Remove"), this);
    connect(m_presetRemoveButton, &QPushButton::released,
            this, &PmMatchTab::onPresetRemove, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetRemoveButton);

    mainLayout->addRow(obs_module_text("Preset: "), presetLayout);

    // divider line 1
    QFrame *line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    mainLayout->addRow(line1);

    // image path display and browse button
    QHBoxLayout *imgPathSubLayout = new QHBoxLayout;
    imgPathSubLayout->setContentsMargins(0, 0, 0, 0);

    m_imgPathEdit = new QLineEdit(this);
    m_imgPathEdit->setReadOnly(true);
    imgPathSubLayout->addWidget(m_imgPathEdit);

    QPushButton *browseImgPathBtn = new QPushButton(
        obs_module_text("Browse"), this);
    connect(browseImgPathBtn, &QPushButton::released,
            this, &PmMatchTab::onBrowseButtonReleased);
    imgPathSubLayout->addWidget(browseImgPathBtn);

    mainLayout->addRow(obs_module_text("Image: "), imgPathSubLayout);

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
    connect(m_maskModeCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onColorComboIndexChanged()));
    colorSubLayout->addWidget(m_maskModeCombo);

    m_maskModeDisplay = new QLabel(this);
    m_maskModeDisplay->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorSubLayout->addWidget(m_maskModeDisplay);

    mainLayout->addRow(obs_module_text("Mask Mode: "), colorSubLayout);

    // match location
    QHBoxLayout *matchLocSubLayout = new QHBoxLayout;
    matchLocSubLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *posXLabel = new QLabel("X = ", this);
    matchLocSubLayout->addWidget(posXLabel);
    m_posXBox = new QSpinBox(this);
    m_posXBox->setSuffix(" px");
    m_posXBox->setMinimum(0);
    m_posXBox->setSingleStep(1);
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
    connect(m_posYBox, SIGNAL(valueChanged(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    matchLocSubLayout->addWidget(m_posYBox);

    mainLayout->addRow(obs_module_text("Location: "), matchLocSubLayout);

    // per pixel error tolerance
    m_perPixelErrorBox = new QDoubleSpinBox(this);
    m_perPixelErrorBox->setSuffix(" %");
    m_perPixelErrorBox->setRange(0.0, 100.0);
    m_perPixelErrorBox->setSingleStep(1.0);
    m_perPixelErrorBox->setDecimals(1);
    connect(m_perPixelErrorBox, SIGNAL(valueChanged(double)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);

    mainLayout->addRow(
        obs_module_text("Allowed Pixel Error: "), m_perPixelErrorBox);

    // total match error threshold
    m_totalMatchThreshBox = new QDoubleSpinBox(this);
    m_totalMatchThreshBox->setSuffix(" %");
    m_totalMatchThreshBox->setRange(1.0, 100.0);
    m_totalMatchThreshBox->setSingleStep(1.0);
    m_totalMatchThreshBox->setDecimals(1);
    connect(m_totalMatchThreshBox, SIGNAL(valueChanged(double)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);

    mainLayout->addRow(
        obs_module_text("Global Match Threshold: "), m_totalMatchThreshBox);

    // match result label
    m_matchResultDisplay = new QLabel("--", this);
    m_matchResultDisplay->setTextFormat(Qt::RichText);
    mainLayout->addRow(
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

    // divider line 2
    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    mainLayout->addRow(line2);

    mainLayout->addRow(
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
    connect(m_matchImgScaleCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    m_previewScaleStack->insertWidget(
        int(PmPreviewMode::MatchImage), m_matchImgScaleCombo);

    mainLayout->addRow(
        obs_module_text("Preview Scale: "), m_previewScaleStack);

    // image/match display area

    m_filterDisplay = new OBSQTDisplay(this);
    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(m_filterDisplay->GetDisplay(),
                          PmMatchTab::drawPreview,
                          this);
    };
    connect(m_filterDisplay, &OBSQTDisplay::DisplayCreated,
            addDrawCallback);
    connect(m_filterDisplay, &OBSQTDisplay::destroyed,
            this, &PmMatchTab::onDestroy, Qt::DirectConnection);
    mainLayout->addRow(m_filterDisplay);

    // notification label
    m_notifyLabel = new QLabel(this);
    mainLayout->addRow(m_notifyLabel);

    setLayout(mainLayout);

    // core signals -> local slots
    connect(m_core, &PmCore::sigImgSuccess,
            this, &PmMatchTab::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
            this, &PmMatchTab::onImgFailed, Qt::QueuedConnection);

    connect(m_core, &PmCore::sigMatchPresetsChanged,
            this, &PmMatchTab::onPresetsChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigMatchPresetStateChanged,
            this, &PmMatchTab::onPresetStateChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMatchResults,
            this, &PmMatchTab::onNewMatchResults, Qt::QueuedConnection);

    // local signals -> core slots
    connect(this, &PmMatchTab::sigNewUiConfig,
            m_core, &PmCore::onNewMatchConfig, Qt::QueuedConnection);
    connect(this, &PmMatchTab::sigSaveMatchPreset,
            m_core, &PmCore::onSaveMatchPreset, Qt::QueuedConnection);
    connect(this, &PmMatchTab::sigSelectActiveMatchPreset,
            m_core, &PmCore::onSelectActiveMatchPreset, Qt::QueuedConnection);
    connect(this, &PmMatchTab::sigRemoveMatchPreset,
            m_core, &PmCore::onRemoveMatchPreset, Qt::QueuedConnection);

    // finish init
    onNewMatchResults(m_core->results());
    configToUi(m_core->matchConfig());
    onColorComboIndexChanged();
    onPresetsChanged();
    onPresetStateChanged();
    onConfigUiChanged();
}

PmMatchTab::~PmMatchTab()
{
    while(m_rendering) {
        QThread::sleep(1);
    }
    //onDestroy(nullptr);
}

void PmMatchTab:: drawPreview(void *data, uint32_t cx, uint32_t cy)
{
    auto dialog = static_cast<PmMatchTab*>(data);
    auto core = dialog->m_core;

    if (!core || !core->activeFilterRef().isValid()) return;

    dialog->m_rendering = true;
    auto config = core->matchConfig();
    if (config.previewMode == PmPreviewMode::MatchImage) {
        dialog->drawMatchImage();
    } else {
        dialog->drawEffect();
    }
    dialog->m_rendering = false;

    UNUSED_PARAMETER(cx);
    UNUSED_PARAMETER(cy);
}

void PmMatchTab::drawEffect()
{
    auto config = m_core->matchConfig();
    auto filterRef = m_core->activeFilterRef();
    auto renderSrc = filterRef.filter();

    if (!renderSrc) return;

    float orthoLeft, orthoBottom, orthoRight, orthoTop;
    int vpLeft, vpBottom, vpWidth, vpHeight;

    if (config.previewMode == PmPreviewMode::Video) {
        int cx = int(m_prevResults.baseWidth);
        int cy = int(m_prevResults.baseHeight);
        int scaledCx = int(cx * config.previewVideoScale);
        int scaledCy = int(cy * config.previewVideoScale);

        orthoLeft = 0.f;
        orthoBottom = 0.f;
        orthoRight = cx;
        orthoTop = cy;

        float scale;
        GetScaleAndCenterPos(cx, cy, scaledCx, scaledCy, vpLeft, vpBottom, scale);
        vpWidth = scaledCx;
        vpHeight = scaledCy;
    } else if (config.previewMode == PmPreviewMode::Region) {
        const auto &results = m_prevResults;
        orthoLeft = config.roiLeft;
        orthoBottom = config.roiBottom;
        orthoRight = config.roiLeft + int(results.matchImgWidth);
        orthoTop = config.roiBottom + int(results.matchImgHeight);

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

void PmMatchTab::drawMatchImage()
{
    auto config = m_core->matchConfig();
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

void PmMatchTab::maskModeChanged(PmMaskMode mode, QColor color)
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

void PmMatchTab::configToUi(const PmMatchConfig &config)
{
    blockSignals(true);

    roiRangesChanged(m_prevResults.baseWidth, m_prevResults.baseHeight, 0, 0);

    m_imgPathEdit->setText(config.matchImgFilename.data());
    m_maskModeCombo->setCurrentIndex(int(config.maskMode));
    m_posXBox->setValue(config.roiLeft);
    m_posYBox->setValue(config.roiBottom);
    m_perPixelErrorBox->setValue(double(config.perPixelErrThresh));
    m_totalMatchThreshBox->setValue(double(config.totalMatchThresh));
    m_previewModeButtons->button(int(config.previewMode))->setChecked(true);
    m_videoScaleCombo->setCurrentIndex(
        m_videoScaleCombo->findData(config.previewVideoScale));
    m_regionScaleCombo->setCurrentIndex(
        m_regionScaleCombo->findData(config.previewRegionScale));
    m_matchImgScaleCombo->setCurrentIndex(
        m_matchImgScaleCombo->findData(config.previewMatchImageScale));
    blockSignals(false);
}

void PmMatchTab::closeEvent(QCloseEvent *e)
{
    obs_display_remove_draw_callback(
        m_filterDisplay->GetDisplay(), PmMatchTab::drawPreview, this);
    QWidget::closeEvent(e);
}

void PmMatchTab::onColorComboIndexChanged()
{
    // send color mode to core? obs data API?
    PmMaskMode mode = PmMaskMode(m_maskModeCombo->currentIndex());
    maskModeChanged(mode, QColor());
    onConfigUiChanged();
}

void PmMatchTab::onBrowseButtonReleased()
{
    static const QString filter
        = "PNG (*.png);; JPEG (*.jpg *.jpeg);; BMP (*.bmp);; All files (*.*)";

    QString curPath
        = QFileInfo(m_core->matchImgFilename().data()).absoluteDir().path();

    QString path = QFileDialog::getOpenFileName(
        this, obs_module_text("Open an image file"), curPath,
        filter);
    if (!path.isEmpty()) {
        PmMatchConfig config = m_core->matchConfig();
        config.matchImgFilename = path.toUtf8().data();
        emit sigNewUiConfig(config);
    }
}

void PmMatchTab::onImgSuccess(std::string filename)
{
    m_imgPathEdit->setText(filename.data());
    m_imgPathEdit->setStyleSheet("");
}

void PmMatchTab::onImgFailed(std::string filename)
{
    m_imgPathEdit->setText(
        filename.size() ? QString("[FAILED] %1").arg(filename.data()) : "");
    m_imgPathEdit->setStyleSheet("color: red");
}

void PmMatchTab::updateFilterDisplaySize(
    const PmMatchConfig &config, const PmMatchResults &results)
{
    int cx, cy;
    if (config.previewMode == PmPreviewMode::Video) {
        if (!m_core->activeFilterRef().isValid()) {
            cx = 0;
            cy = 0;
        } else {
            float scale = config.previewVideoScale;
            cx = int(results.baseWidth * scale);
            cy = int(results.baseHeight * scale);
        }
    } else if (config.previewMode == PmPreviewMode::Region) {
        if (!m_core->activeFilterRef().isValid()) {
            cx = 0;
            cy = 0;
        } else {
            float scale = config.previewRegionScale;
            cx = int(results.matchImgWidth * scale);
            cy = int(results.matchImgHeight * scale);
        }
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

void PmMatchTab::roiRangesChanged(
    int baseWidth, int baseHeight, int imgWidth, int imgHeight)
{
    m_posXBox->setMaximum(baseWidth - imgWidth);
    m_posYBox->setMaximum(baseHeight - imgHeight);
}

void PmMatchTab::onNewMatchResults(PmMatchResults results)
{
    if (m_prevResults.baseWidth != results.baseWidth
     || m_prevResults.matchImgWidth != results.matchImgWidth
     || m_prevResults.baseHeight != results.baseHeight
     || m_prevResults.matchImgHeight != results.matchImgHeight) {
        roiRangesChanged(results.baseWidth, results.baseHeight,
                         results.matchImgWidth, results.matchImgHeight);
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

    updateFilterDisplaySize(m_core->matchConfig(), results);
    m_prevResults = results;
}

void PmMatchTab::onConfigUiChanged()
{
    PmMatchConfig config;
    config.matchImgFilename = m_imgPathEdit->text().toUtf8().data();
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

void PmMatchTab::onDestroy(QObject *obj)
{
    while(m_rendering) {
        QThread::sleep(1);
    }
    obs_display_remove_draw_callback(
                m_filterDisplay->GetDisplay(), PmMatchTab::drawPreview, this);
    UNUSED_PARAMETER(obj);
}

void PmMatchTab::onPresetsChanged()
{
    PmMatchPresets presets = m_core->matchPresets();
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    for (auto pair: presets) {
        m_presetCombo->addItem(pair.first.data());
    }
    m_presetCombo->blockSignals(false);
}

void PmMatchTab::onPresetStateChanged()
{
    std::string activePreset = m_core->activeMatchPresetName();
    bool dirty = m_core->matchConfigDirty();

    m_presetCombo->blockSignals(true);
    int findPlaceholder = m_presetCombo->findText(k_unsavedPresetStr);
    if (activePreset.empty()) {
        if (findPlaceholder == -1) {
            m_presetCombo->addItem(k_unsavedPresetStr);
        }
        m_presetCombo->setCurrentText(k_unsavedPresetStr);
    } else {
        if (findPlaceholder != -1) {
            m_presetCombo->removeItem(findPlaceholder);
        }
        m_presetCombo->setCurrentText(activePreset.data());
    }
    m_presetCombo->blockSignals(false);

    m_presetRemoveButton->setEnabled(!activePreset.empty());
    m_presetSaveButton->setEnabled(dirty);
}

void PmMatchTab::onPresetSelected()
{
    std::string selPreset = m_presetCombo->currentText().toUtf8().data();
    PmMatchConfig presetConfig = m_core->matchPresetByName(selPreset);
    emit sigSelectActiveMatchPreset(selPreset);
    configToUi(presetConfig);
    updateFilterDisplaySize(presetConfig, m_prevResults);
}

void PmMatchTab::onPresetSave()
{
    std::string presetName = m_core->activeMatchPresetName();
    if (presetName.empty()) {
        onPresetSaveAs();
    } else {
        emit sigSaveMatchPreset(presetName);
    }
}

void PmMatchTab::onPresetSaveAs()
{
    bool ok;
    QString presetNameQstr = QInputDialog::getText(
        this, obs_module_text("Save Preset"), obs_module_text("Enter Name: "),
        QLineEdit::Normal, QString(), &ok);

    std::string presetName = presetNameQstr.toUtf8().data();

    if (!ok || presetName.empty()) return;

    if (m_core->activeMatchPresetName() != presetName
     && m_core->matchPresetExists(presetName)) {
        int ret = QMessageBox::warning(this, "Preset Exists",
            QString("Overwrite preset \"%1\"?").arg(presetNameQstr),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if (ret != QMessageBox::Yes) return;
    }

    emit sigSaveMatchPreset(presetName);
}

void PmMatchTab::onConfigReset()
{
    PmMatchConfig config; // defaults
    configToUi(config);
    updateFilterDisplaySize(config, m_prevResults);
    emit sigNewUiConfig(config);
    emit sigSelectActiveMatchPreset("");
}

void PmMatchTab::onPresetRemove()
{
    auto presets = m_core->matchPresets();
    std::string oldPreset = m_core->activeMatchPresetName();
    emit sigRemoveMatchPreset(oldPreset);
    for (auto p: presets) {
        if (p.first != oldPreset) {
            configToUi(p.second);
            emit sigSelectActiveMatchPreset(p.first);
            break;
        }
    }
}
