#include "pm-match-config-widget.hpp"
#include "pm-core.hpp"
#include "pm-filter.h"

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
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>

#include <obs-module.h>


const char* PmMatchConfigWidget::k_failedImgStr
    = obs_module_text("[FAILED]");

PmMatchConfigWidget::PmMatchConfigWidget(PmCore *pixelMatcher, QWidget *parent)
: QWidget(parent)
, m_core(pixelMatcher)
{   
#if 0
    // checker brush
    {
        // TODO: optimize
        const size_t half=8;
        const size_t full=half*2;
        uchar bits[full*full];
        for (size_t x = 0; x < half; ++x) {
            for (size_t y = 0; y < half; ++y) {
                bits[y*full+x] = 0xFF;
                bits[y*full+half+x] = 0;
                bits[(y+half)*full+x] = 0;
                bits[(y+half)*full+half+x] = 0xFF;
            }
        }
        m_checkerPixMap = QPixmap::fromImage(QImage(bits, full, full, QImage::Format_Grayscale8));
    }
#endif

    // main layout
    QFormLayout *mainLayout = new QFormLayout;
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // index and label
    m_configCaption = new QLabel(this);
    m_labelEdit = new QLineEdit(this);   
    connect(m_labelEdit, &QLineEdit::textEdited,
            this, &PmMatchConfigWidget::onConfigUiChanged, Qt::QueuedConnection);
    mainLayout->addRow(m_configCaption, m_labelEdit);

    // image path display and browse button
    QHBoxLayout *imgPathSubLayout = new QHBoxLayout;
    imgPathSubLayout->setContentsMargins(0, 0, 0, 0);

    m_imgPathEdit = new QLineEdit(this);
    m_imgPathEdit->setReadOnly(true);
    imgPathSubLayout->addWidget(m_imgPathEdit);

    QPushButton *browseImgPathBtn = new QPushButton(
        obs_module_text("Browse"), this);
    browseImgPathBtn->setFocusPolicy(Qt::NoFocus);
    connect(browseImgPathBtn, &QPushButton::released,
            this, &PmMatchConfigWidget::onBrowseButtonReleased);
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
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    colorSubLayout->addWidget(m_maskModeCombo);

    m_maskModeDisplay = new QLabel(this);
    m_maskModeDisplay->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorSubLayout->addWidget(m_maskModeDisplay);

    m_pickColorButton = new QPushButton(obs_module_text("Pick"), this);
    connect(m_pickColorButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onPickColorButtonReleased, Qt::QueuedConnection);
    colorSubLayout->addWidget(m_pickColorButton);

    mainLayout->addRow(obs_module_text("Mask Mode: "), colorSubLayout);

    // match location
    QHBoxLayout *matchLocSubLayout = new QHBoxLayout;
    matchLocSubLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *posXLabel = new QLabel("X = ", this);
    matchLocSubLayout->addWidget(posXLabel);
    m_posXBox = new QSpinBox(this);
    m_posXBox->setSuffix(" px");
    m_posXBox->setRange(0, std::numeric_limits<int>::max());
    m_posXBox->setSingleStep(1);
    connect(m_posXBox, SIGNAL(valueChanged(int)),
            this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    matchLocSubLayout->addWidget(m_posXBox);
    matchLocSubLayout->addItem(new QSpacerItem(10, 1));

    QLabel *posYLabel = new QLabel("Y = ", this);
    matchLocSubLayout->addWidget(posYLabel);
    m_posYBox = new QSpinBox(this);
    m_posYBox->setSuffix(" px");
    m_posYBox->setRange(0, std::numeric_limits<int>::max());
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

    // divider line
    QFrame* dividerLine = new QFrame();
    dividerLine->setFrameShape(QFrame::HLine);
    dividerLine->setFrameShadow(QFrame::Sunken);
    mainLayout->addRow(dividerLine);

    // match result label
    m_matchResultDisplay = new QLabel("--", this);
    m_matchResultDisplay->setTextFormat(Qt::RichText);
    mainLayout->addRow(
        obs_module_text("Match Result: "), m_matchResultDisplay);

    setLayout(mainLayout);

    // core signals -> local slots
    connect(m_core, &PmCore::sigImgSuccess,
            this, &PmMatchConfigWidget::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
            this, &PmMatchConfigWidget::onImgFailed, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMatchResults,
            this, &PmMatchConfigWidget::onNewMatchResults, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigChangedMatchConfig,
            this, &PmMatchConfigWidget::onChangedMatchConfig, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigSelectMatchIndex,
            this, &PmMatchConfigWidget::onSelectMatchIndex, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMultiMatchConfigSize,
            this, &PmMatchConfigWidget::onNewMultiMatchConfigSize, Qt::QueuedConnection);

    // local signals -> core
    connect(this, &PmMatchConfigWidget::sigChangedMatchConfig,
            m_core, &PmCore::onChangedMatchConfig, Qt::QueuedConnection);

    // finish init
    size_t selIdx = m_core->selectedConfigIndex();
    onNewMultiMatchConfigSize(m_core->multiMatchConfigSize());
    onSelectMatchIndex(selIdx, m_core->matchConfig(selIdx));
    onNewMatchResults(selIdx, m_core->matchResults(selIdx));
}

PmMatchConfigWidget::~PmMatchConfigWidget()
{
}

void PmMatchConfigWidget::onSelectMatchIndex(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;
    m_configCaption->setText(
        QString(obs_module_text("Config #%1:")).arg(matchIndex+1));

    onChangedMatchConfig(matchIndex, cfg);
    bool existingSelected = matchIndex < m_multiConfigSz;
    setEnabled(existingSelected);

    if (existingSelected && m_core->runningEnabled()) {
        if (m_core->hasFilename(matchIndex)) {
            std::string matchImgFilename = m_core->matchImgFilename(matchIndex);
            if (m_core->matchImageLoaded(matchIndex)) {
                onImgFailed(matchIndex, matchImgFilename);
            } else {
                onImgSuccess(matchIndex, matchImgFilename, 
                             m_core->matchImage(matchIndex));
            }
        }
    } else {
        m_matchResultDisplay->setText(obs_module_text("N/A"));
    }

    onConfigUiChanged();
}

void PmMatchConfigWidget::onNewMultiMatchConfigSize(size_t sz)
{
    m_multiConfigSz = sz;
    setEnabled(m_matchIndex < m_multiConfigSz);
}


void PmMatchConfigWidget::maskModeChanged(PmMaskMode mode, vec3 customColor)
{
    m_pickColorButton->setVisible(mode == PmMaskMode::CustomClrMode);

    if (mode == PmMaskMode::AlphaMode) {
        m_maskModeDisplay->setVisible(false);
        return;
    }  else {
        m_maskModeDisplay->setVisible(true);
    }

    QColor color, textColor = Qt::black;
    QString textLabel;

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
    case PmMaskMode::CustomClrMode:
        color = toQColor(customColor);
        textColor = toQColor(customColor);
        textColor = QColor(
            255-textColor.red(), 255-textColor.green(), 255-textColor.blue());
        break;
    default:
        break;
    }
    m_maskModeDisplay->setStyleSheet(
        QString("background-color :%1; color: %2;")
            .arg(color.name(QColor::HexArgb))
            .arg(textColor.name(QColor::HexArgb)));
    m_maskModeDisplay->setText(color.name(QColor::HexArgb));
}

void PmMatchConfigWidget::onChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    m_labelEdit->blockSignals(true);
    m_labelEdit->setText(cfg.label.data());
    m_labelEdit->blockSignals(false);

    m_imgPathEdit->blockSignals(true);
    m_imgPathEdit->setText(cfg.matchImgFilename.data());
    m_imgPathEdit->blockSignals(false);

    m_maskModeCombo->blockSignals(true);
    m_maskModeCombo->setCurrentIndex(int(cfg.maskMode));
    m_maskModeCombo->blockSignals(false);

    m_customColor = cfg.filterCfg.mask_color;

    m_posXBox->blockSignals(true);
    m_posXBox->setValue(cfg.filterCfg.roi_left);
    m_posXBox->blockSignals(false);

    m_posYBox->blockSignals(true);;
    m_posYBox->setValue(cfg.filterCfg.roi_bottom);
    m_posYBox->blockSignals(false);

    m_perPixelErrorBox->blockSignals(true);
    m_perPixelErrorBox->setValue(double(cfg.filterCfg.per_pixel_err_thresh));
    m_perPixelErrorBox->blockSignals(false);

    m_totalMatchThreshBox->blockSignals(true);
    m_totalMatchThreshBox->setValue(double(cfg.totalMatchThresh));
    m_totalMatchThreshBox->blockSignals(false);

    roiRangesChanged(m_prevResults.baseWidth, m_prevResults.baseHeight, 0, 0);
    maskModeChanged(cfg.maskMode, m_customColor);

    blockSignals(false);
}

void PmMatchConfigWidget::onPickColorButtonReleased()
{
    QColor startColor = toQColor(m_customColor);
    //uint32_t newColor = toUInt32(QColorDialog::getColor(startColor, this));
    vec3 newColor = toVec3(QColorDialog::getColor(startColor, this));
    if (m_customColor.x != newColor.x 
     || m_customColor.y != newColor.y
     || m_customColor.z != newColor.z) {
        m_customColor = newColor;
        onConfigUiChanged();
    }
}

void PmMatchConfigWidget::onBrowseButtonReleased()
{
    auto config = m_core->matchConfig(m_matchIndex);

    static const QString filter
        = "PNG (*.png);; JPEG (*.jpg *.jpeg);; BMP (*.bmp);; All files (*.*)";

    QString curPath
        = QFileInfo(config.matchImgFilename.data()).absoluteDir().path();

    QString path = QFileDialog::getOpenFileName(
        this, obs_module_text("Open an image file"), curPath, filter);
    if (!path.isEmpty()) {
        config.matchImgFilename = path.toUtf8().data();
        emit sigChangedMatchConfig(m_matchIndex, config);
    }
}

void PmMatchConfigWidget::onImgSuccess(
    size_t matchIndex, std::string filename, QImage img)
{   
    if (matchIndex != m_matchIndex) return;

    m_imgPathEdit->setText(filename.data());
    m_imgPathEdit->setStyleSheet("");
}

void PmMatchConfigWidget::onImgFailed(size_t matchIndex, std::string filename)
{
    if (matchIndex != m_matchIndex) return;

    QString imgStr;
    if (filename.size()) {
        imgStr = QString("%1 %2").arg(k_failedImgStr).arg(filename.data());
    }
    m_imgPathEdit->setText(imgStr);
    m_imgPathEdit->setStyleSheet("color: red");
}

QColor PmMatchConfigWidget::toQColor(vec3 val)
{
    return QColor(int(val.x * 255.f), int(val.y * 255.f), int(val.z * 255.f));
}

vec3 PmMatchConfigWidget::toVec3(QColor val)
{
    vec3 ret;
    ret.x = val.red() / 255.f;
    ret.y = val.green() / 255.f;
    ret.z = val.blue() / 255.f;
    return ret;
}

void PmMatchConfigWidget::roiRangesChanged(
    uint32_t baseWidth, uint32_t baseHeight,
    uint32_t imgWidth, uint32_t imgHeight)
{
    if (baseWidth == 0 || baseHeight == 0)
        return;
    m_posXBox->setMaximum(int(baseWidth - imgWidth));
    m_posYBox->setMaximum(int(baseHeight - imgHeight));
}

void PmMatchConfigWidget::onNewMatchResults(
    size_t matchIdx, PmMatchResults results)
{
    if (matchIdx != m_matchIndex) return;

    if (m_prevResults.baseWidth != results.baseWidth
     || m_prevResults.baseHeight != results.baseHeight
     || m_prevResults.matchImgWidth != results.matchImgWidth
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

    m_prevResults = results;
}

void PmMatchConfigWidget::onConfigUiChanged()
{
    PmMatchConfig config = m_core->matchConfig(m_matchIndex);
    
    std::string label = m_labelEdit->text().toUtf8().data();

    std::string filename = m_imgPathEdit->text().toUtf8().data();
    size_t failedMarker = filename.find(k_failedImgStr);
    if (failedMarker == 0) {
        filename.erase(failedMarker, strlen(k_failedImgStr)+1);
    }
    
    config.label = label;
    config.matchImgFilename = filename;
    config.filterCfg.roi_left = m_posXBox->value();
    config.filterCfg.roi_bottom = m_posYBox->value();
    config.filterCfg.per_pixel_err_thresh = float(m_perPixelErrorBox->value());
    config.totalMatchThresh = float(m_totalMatchThreshBox->value());

    config.filterCfg.mask_alpha = (config.maskMode == PmMaskMode::AlphaMode);
    config.maskMode = PmMaskMode(m_maskModeCombo->currentIndex());
    //config.filterCfg.mask_color = m_customColor;
    float r = 0.f, g = 0.f, b = 0.f;
    switch (config.maskMode) {
    case PmMaskMode::AlphaMode:
    case PmMaskMode::BlackMode:
        config.filterCfg.mask_color = { 0.f, 0.f, 0.f };
        break;
    case PmMaskMode::GreenMode:
        config.filterCfg.mask_color = { 0.f, 1.f, 0.f };
        break;
    case PmMaskMode::MagentaMode:
        config.filterCfg.mask_color = { 1.f, 0.f, 1.f };
        break;
    case PmMaskMode::CustomClrMode:
        config.filterCfg.mask_color = m_customColor;
        break;
    }

    maskModeChanged(config.maskMode, config.filterCfg.mask_color);
    emit sigChangedMatchConfig(m_matchIndex, config);
}

#if 0
QColor PmMatchConfigWidget::toQColor(uint32_t val)
{
    uint8_t* colorBytes = reinterpret_cast<uint8_t*>(&val);
#if 0
#if PM_LITTLE_ENDIAN
    return QColor(int(colorBytes[2]), int(colorBytes[1]),
        int(colorBytes[0]), int(colorBytes[3]));

#else
    return QColor(int(colorBytes[1]), int(colorBytes[2]),
        int(colorBytes[3]), int(colorBytes[0]));
#endif
#endif
}

uint32_t PmMatchConfigWidget::toUInt32(QColor val)
{
#if PM_LITTLE_ENDIAN
    return uint32_t(val.alpha() << 24) | uint32_t(val.red() << 16)
        | uint32_t(val.green() << 8) | uint32_t(val.blue());
#else
    return uint32_t(val.blue() << 24) | uint32_t(val.green() << 16)
        | uint32_t(val.red() << 8) | uint32_t(val.alpha());
#endif
}
#endif