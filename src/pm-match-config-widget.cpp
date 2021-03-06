#include "pm-match-config-widget.hpp"
#include "pm-core.hpp"
#include "pm-filter.h"

#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QStackedWidget>
#include <QProcess>

#include <obs-module.h>


const char* PmMatchConfigWidget::k_failedImgStr
    = obs_module_text("[FAILED]");

PmMatchConfigWidget::PmMatchConfigWidget(PmCore *pixelMatcher, QWidget *parent)
: PmSpoilerWidget(parent)
, m_core(pixelMatcher)
{   
    const Qt::ConnectionType qc = Qt::QueuedConnection;

    // main layout
    QFormLayout *mainLayout = new QFormLayout;
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // image control buttons: when not capturing
    QHBoxLayout *imgControlLayout0 = new QHBoxLayout;
    imgControlLayout0->setContentsMargins(0, 0, 0, 0);

    m_captureBeginButton = new QPushButton(obs_module_text("Capture"), this);
    m_captureBeginButton->setFocusPolicy(Qt::NoFocus);
    connect(m_captureBeginButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onCaptureBeginButtonReleased);
    imgControlLayout0->addWidget(m_captureBeginButton);

    m_openFileButton = new QPushButton(obs_module_text("Open File"), this);
    m_openFileButton->setFocusPolicy(Qt::NoFocus);
    connect(m_openFileButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onOpenFileButtonReleased);
    imgControlLayout0->addWidget(m_openFileButton);
    imgControlLayout0->setStretchFactor(m_openFileButton, 1);

    m_editFileButton = new QPushButton(obs_module_text("Edit"), this);
    m_editFileButton->setFocusPolicy(Qt::NoFocus);
    m_editFileButton->setEnabled(false);
    imgControlLayout0->addWidget(m_editFileButton);
    imgControlLayout0->setStretchFactor(m_editFileButton, 1);

    m_openFolderButton = new QPushButton(obs_module_text("Open Folder"), this);
    m_openFolderButton->setFocusPolicy(Qt::NoFocus);
    connect(m_openFolderButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onOpenFolderButtonReleased);
    imgControlLayout0->addWidget(m_openFolderButton);
    imgControlLayout0->setStretchFactor(m_openFolderButton, 1);

    m_refreshButton = new QPushButton(obs_module_text("Refresh"), this);
    m_refreshButton->setFocusPolicy(Qt::NoFocus);
    connect(m_refreshButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onRefreshButtonReleased);
    imgControlLayout0->addWidget(m_refreshButton);
    imgControlLayout0->setStretchFactor(m_refreshButton, 1);

    // image control buttons: during capture
    QHBoxLayout* imgControlLayout1 = new QHBoxLayout;
    imgControlLayout1->setContentsMargins(0, 0, 0, 0);

    m_captureAcceptButton = new QPushButton(
        obs_module_text("Accept Capture"), this);
    m_captureAcceptButton->setFocusPolicy(Qt::NoFocus);
    connect(m_captureAcceptButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onCaptureAcceptButtonReleased);
    imgControlLayout1->addWidget(m_captureAcceptButton);

    m_captureAutomaskButton = new QPushButton(
        obs_module_text("Begin Auto-Mask"), this);
    m_captureAutomaskButton->setCheckable(true);
    m_captureAutomaskButton->setFocusPolicy(Qt::NoFocus);
    connect(m_captureAutomaskButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onCaptureAutomaskButtonReleased);
    imgControlLayout1->addWidget(m_captureAutomaskButton);

    m_captureCancelButton = new QPushButton(
        obs_module_text("Cancel Capture"), this);
    m_captureCancelButton->setFocusPolicy(Qt::NoFocus);
    connect(m_captureCancelButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onCaptureCancelButtonReleased);
    imgControlLayout1->addWidget(m_captureCancelButton);

    // finish image controls widgets and stack
    QWidget* buttonsPage0 = new QWidget(this);
    QWidget* buttonsPage1 = new QWidget(this);
    buttonsPage0->setLayout(imgControlLayout0);
    buttonsPage1->setLayout(imgControlLayout1);
    
    m_buttonsStack = new QStackedWidget(this);
    m_buttonsStack->addWidget(buttonsPage0);
    m_buttonsStack->addWidget(buttonsPage1);
    mainLayout->addRow(m_buttonsStack);

    // some spacing
    QFrame* spacer = new QFrame(this);
    spacer->setFrameShape(QFrame::HLine);
    //spacer->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(spacer);

    // image path display
    m_imgPathEdit = new QLineEdit(this);
    m_imgPathEdit->setReadOnly(true);
    mainLayout->addRow(obs_module_text("Image: "), m_imgPathEdit);

    // color mode selection
    QHBoxLayout *colorSubLayout = new QHBoxLayout;
    colorSubLayout->setContentsMargins(0, 0, 0, 0);

    m_maskModeCombo = new QComboBox(this);
    m_maskModeCombo->insertItem(
        int(PmMaskMode::AlphaMode), obs_module_text("Alpha"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::GreenMode), obs_module_text("Green"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::MagentaMode), obs_module_text("Magenta"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::BlackMode), obs_module_text("Black"));
    m_maskModeCombo->insertItem(
        int(PmMaskMode::CustomClrMode), obs_module_text("Custom"));
    connect(m_maskModeCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onConfigUiChanged()), qc);
    colorSubLayout->addWidget(m_maskModeCombo);

    m_maskModeDisplay = new QLabel(this);
    m_maskModeDisplay->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorSubLayout->addWidget(m_maskModeDisplay);

    m_pickColorButton = new QPushButton(obs_module_text("Pick"), this);
    connect(m_pickColorButton, &QPushButton::released,
            this, &PmMatchConfigWidget::onPickColorButtonReleased, qc);
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
            this, SLOT(onConfigUiChanged()), qc);
    matchLocSubLayout->addWidget(m_posXBox);
    matchLocSubLayout->addItem(new QSpacerItem(10, 1));

    QLabel *posYLabel = new QLabel("Y = ", this);
    matchLocSubLayout->addWidget(posYLabel);
    m_posYBox = new QSpinBox(this);
    m_posYBox->setSuffix(" px");
    m_posYBox->setRange(0, std::numeric_limits<int>::max());
    m_posYBox->setSingleStep(1);
    connect(m_posYBox, SIGNAL(valueChanged(int)),
        this, SLOT(onConfigUiChanged()), qc);
    matchLocSubLayout->addWidget(m_posYBox);

    mainLayout->addRow(obs_module_text("Location: "), matchLocSubLayout);

    // per pixel error tolerance
    m_perPixelErrorBox = new QDoubleSpinBox(this);
    m_perPixelErrorBox->setSuffix(" %");
    m_perPixelErrorBox->setRange(0.0, 100.0);
    m_perPixelErrorBox->setSingleStep(1.0);
    m_perPixelErrorBox->setDecimals(1);
    connect(m_perPixelErrorBox, SIGNAL(valueChanged(double)),
            this, SLOT(onConfigUiChanged()), qc);

    mainLayout->addRow(
        obs_module_text("Allowed Pixel Error: "), m_perPixelErrorBox);

    // total match error threshold
    m_totalMatchThreshBox = new QDoubleSpinBox(this);
    m_totalMatchThreshBox->setSuffix(" %");
    m_totalMatchThreshBox->setRange(1.0, 100.0);
    m_totalMatchThreshBox->setSingleStep(1.0);
    m_totalMatchThreshBox->setDecimals(1);
    connect(m_totalMatchThreshBox, SIGNAL(valueChanged(double)),
            this, SLOT(onConfigUiChanged()), qc);

    mainLayout->addRow(
        obs_module_text("Match Threshold: "), m_totalMatchThreshBox);

    // invert result checkbox
    m_invertResultCheckbox = new QCheckBox(this);
    m_invertResultCheckbox->setFocusPolicy(Qt::NoFocus);
    connect(m_invertResultCheckbox, &QCheckBox::toggled, this,
            &PmMatchConfigWidget::onConfigUiChanged, qc);

    mainLayout->addRow(obs_module_text("Invert Result: "), m_invertResultCheckbox);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    setContentLayout(mainLayout);

    // core signals -> local slots
    connect(m_core, &PmCore::sigMatchImageLoadSuccess,
            this, &PmMatchConfigWidget::onMatchImageLoadSuccess, qc);
    connect(m_core, &PmCore::sigMatchImageLoadFailed,
            this, &PmMatchConfigWidget::onMatchImageLoadFailed, qc);
    connect(m_core, &PmCore::sigNewMatchResults,
            this, &PmMatchConfigWidget::onNewMatchResults, qc);
    connect(m_core, &PmCore::sigMatchConfigChanged,
            this, &PmMatchConfigWidget::onMatchConfigChanged, qc);
    connect(m_core, &PmCore::sigMatchConfigSelect,
            this, &PmMatchConfigWidget::onMatchConfigSelect, qc);
    connect(m_core, &PmCore::sigMultiMatchConfigSizeChanged,
            this, &PmMatchConfigWidget::onMultiMatchConfigSizeChanged, qc);
    connect(m_core, &PmCore::sigCaptureStateChanged,
            this, &PmMatchConfigWidget::onCaptureStateChanged, qc);
    connect(m_core, &PmCore::sigActiveFilterChanged,
            this, &PmMatchConfigWidget::onActiveFilterChanged, qc);

    // local signals -> core
    connect(this, &PmMatchConfigWidget::sigMatchConfigChanged,
            m_core, &PmCore::onMatchConfigChanged, qc);
    connect(this, &PmMatchConfigWidget::sigCaptureStateChanged,
            m_core, &PmCore::onCaptureStateChanged, qc);
    connect(this, &PmMatchConfigWidget::sigRefreshMatchImage,
            m_core, &PmCore::onMatchImageRefresh, qc);

    // finish state init
    size_t selIdx = m_core->selectedConfigIndex();
    onMultiMatchConfigSizeChanged(m_core->multiMatchConfigSize());
    onMatchConfigSelect(selIdx, m_core->matchConfig(selIdx));
    onNewMatchResults(selIdx, m_core->matchResults(selIdx));
    onCaptureStateChanged(m_core->captureState(), 0, 0);
    onActiveFilterChanged(m_core->activeFilterRef());

    toggleExpand(true);
}

void PmMatchConfigWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;

    onMatchConfigChanged(matchIndex, cfg);
    setEnabled(matchIndex < m_multiConfigSz);

    if (m_core->hasFilename(matchIndex) && m_core->runningEnabled()) {
        std::string matchImgFilename = m_core->matchImgFilename(matchIndex);
        if (m_core->matchImageLoaded(matchIndex)) {
            onMatchImageLoadSuccess(matchIndex, matchImgFilename,
                         m_core->matchImage(matchIndex));
        } else {
            onMatchImageLoadFailed(matchIndex, matchImgFilename);
        }
    } else {
        m_imgPathEdit->setStyleSheet("");
    }
}

void PmMatchConfigWidget::onMultiMatchConfigSizeChanged(size_t sz)
{
    m_multiConfigSz = sz;
    setEnabled(m_matchIndex < m_multiConfigSz);
}

void PmMatchConfigWidget::onActiveFilterChanged(PmFilterRef newAf)
{
    m_captureBeginButton->setEnabled(newAf.isValid());
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

void PmMatchConfigWidget::onMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    setTitle(QString(obs_module_text("Match Config #%1: %2"))
        .arg(matchIdx + 1)
        .arg(cfg.label.data()));

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

    m_posYBox->blockSignals(true);
    m_posYBox->setValue(cfg.filterCfg.roi_bottom);
    m_posYBox->blockSignals(false);

    m_perPixelErrorBox->blockSignals(true);
    m_perPixelErrorBox->setValue(double(cfg.filterCfg.per_pixel_err_thresh));
    m_perPixelErrorBox->blockSignals(false);

    m_totalMatchThreshBox->blockSignals(true);
    m_totalMatchThreshBox->setValue(double(cfg.totalMatchThresh));
    m_totalMatchThreshBox->blockSignals(false);

    m_invertResultCheckbox->blockSignals(true);
    m_invertResultCheckbox->setChecked(cfg.invertResult);
    m_invertResultCheckbox->blockSignals(false);

    roiRangesChanged(m_prevResults.baseWidth, m_prevResults.baseHeight);
    maskModeChanged(cfg.maskMode, m_customColor);

    bool hasMatchFilename = !cfg.matchImgFilename.empty();
    m_openFolderButton->setEnabled(hasMatchFilename);
    m_refreshButton->setEnabled(hasMatchFilename);
}

void PmMatchConfigWidget::onPickColorButtonReleased()
{
    QColor startColor = toQColor(m_customColor);
    vec3 newColor = toVec3(QColorDialog::getColor(startColor, this));
    if (m_customColor.x != newColor.x 
     || m_customColor.y != newColor.y
     || m_customColor.z != newColor.z) {
        m_customColor = newColor;
        onConfigUiChanged();
    }
}

void PmMatchConfigWidget::onCaptureBeginButtonReleased()
{
    auto img = m_core->matchImage(m_matchIndex);
    if (!img.isNull()) {
        // use existing image geometry for the new capture
        auto cfg = m_core->matchConfig(m_matchIndex);
        int begX = cfg.filterCfg.roi_left;
        int begY = cfg.filterCfg.roi_bottom;
        int endX = begX + img.width();
        int endY = begY + img.height();
        emit sigCaptureStateChanged(PmCaptureState::SelectBegin, begX, begY);
        emit sigCaptureStateChanged(PmCaptureState::SelectFinished, endX, endY);
    } else {
        emit sigCaptureStateChanged(PmCaptureState::Activated);
    }
}

void PmMatchConfigWidget::onCaptureAutomaskButtonReleased()
{
    auto capState = m_core->captureState();
    if (capState == PmCaptureState::SelectFinished) {
        emit sigCaptureStateChanged(PmCaptureState::Automask);
    }
    else if (capState == PmCaptureState::Automask) {
        emit sigCaptureStateChanged(PmCaptureState::SelectFinished);
    }
}

void PmMatchConfigWidget::onCaptureAcceptButtonReleased()
{
    emit sigCaptureStateChanged(PmCaptureState::Accepted);
}

void PmMatchConfigWidget::onCaptureCancelButtonReleased()
{
    emit sigCaptureStateChanged(PmCaptureState::Inactive);
}

void PmMatchConfigWidget::onOpenFileButtonReleased()
{
    auto config = m_core->matchConfig(m_matchIndex);

    QString curPath
        = QFileInfo(config.matchImgFilename.data()).absoluteDir().path();

    QString path = QFileDialog::getOpenFileName(
        this, obs_module_text("Open an image file"), curPath, 
        PmConstants::k_imageFilenameFilter);
    if (!path.isEmpty()) {
        std::string str = path.toUtf8().data();
        if (config.matchImgFilename != str) {
            config.matchImgFilename = str;
            config.wasDownloaded = false;
            emit sigMatchConfigChanged(m_matchIndex, config);
        }
    }
}

void PmMatchConfigWidget::onOpenFolderButtonReleased()
{
    auto cfg = m_core->matchConfig(m_matchIndex);
    QFileInfo info(cfg.matchImgFilename.data());
    QString path = info.canonicalFilePath();

#if defined(Q_OS_WIN)
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer", args);
#elif defined(Q_OS_MACOS)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + path + "\"";
    args << "-e";
    args << "end tell";
    QProcess::startDetached("osascript", args);
#else
    // TODO: try to figure out open and select on Linux
    QStringList args;
    args << path;
    QProcess::startDetached("xdg-open", args);
#endif
}

void PmMatchConfigWidget::onRefreshButtonReleased()
{
    emit sigRefreshMatchImage(m_matchIndex);
}

void PmMatchConfigWidget::onMatchImageLoadSuccess(
    size_t matchIndex, std::string filename, QImage img)
{   
    if (matchIndex != m_matchIndex) return;

    m_imgPathEdit->setText(filename.data());
    m_imgPathEdit->setStyleSheet("");

    UNUSED_PARAMETER(img);
}

void PmMatchConfigWidget::onMatchImageLoadFailed(
    size_t matchIndex, std::string filename)
{
    if (matchIndex != m_matchIndex) return;

    QString imgStr;
    if (filename.size()) {
        imgStr = QString("%1 %2").arg(k_failedImgStr).arg(filename.data());
    }
    m_imgPathEdit->setText(imgStr);
    m_imgPathEdit->setStyleSheet("color: red");
}

void PmMatchConfigWidget::onCaptureStateChanged(
    PmCaptureState capState, int x, int y)
{
    switch (capState) {
    case PmCaptureState::Inactive:
        m_buttonsStack->setCurrentIndex(0);
        break;
    case PmCaptureState::Activated:
    case PmCaptureState::SelectBegin:
    case PmCaptureState::SelectMoved:
        m_buttonsStack->setCurrentIndex(1);
        m_captureAcceptButton->setEnabled(false);
        m_captureAutomaskButton->setEnabled(false);
        m_captureCancelButton->setEnabled(true);
        break;
    case PmCaptureState::SelectFinished:
        m_buttonsStack->setCurrentIndex(1);
        m_captureAcceptButton->setEnabled(true);
        m_captureAutomaskButton->setEnabled(true);
        m_captureCancelButton->setEnabled(true);
        break;
    case PmCaptureState::Accepted:
    case PmCaptureState::Automask:
        break;
    }

    if (capState == PmCaptureState::Automask) {
        m_captureAutomaskButton->setChecked(true);
        m_captureAutomaskButton->setText(
            obs_module_text("Auto-mask: Active"));
    } else {
        m_captureAutomaskButton->setChecked(false);
        m_captureAutomaskButton->setText(
            obs_module_text("Begin Auto-mask"));
    }

    UNUSED_PARAMETER(x);
    UNUSED_PARAMETER(y);
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
    uint32_t baseWidth, uint32_t baseHeight)
{
    if (baseWidth == 0 || baseHeight == 0)
        return;
    m_posXBox->setMaximum(std::max(int(baseWidth), m_posXBox->value()));
    m_posYBox->setMaximum(std::max(int(baseHeight), m_posYBox->value()));
}

void PmMatchConfigWidget::onNewMatchResults(
    size_t matchIdx, PmMatchResults results)
{
    if (matchIdx != m_matchIndex) return;

    if (m_prevResults.baseWidth != results.baseWidth
     || m_prevResults.baseHeight != results.baseHeight
     || m_prevResults.matchImgWidth != results.matchImgWidth
     || m_prevResults.matchImgHeight != results.matchImgHeight) {
        roiRangesChanged(results.baseWidth, results.baseHeight);
    }

    m_prevResults = results;
}

void PmMatchConfigWidget::onConfigUiChanged()
{
    PmMatchConfig config = m_core->matchConfig(m_matchIndex);
    
    std::string filename = m_imgPathEdit->text().toUtf8().data();
    size_t failedMarker = filename.find(k_failedImgStr);
    if (failedMarker == 0) {
        filename.erase(failedMarker, strlen(k_failedImgStr)+1);
    }

    if (config.wasDownloaded && config.matchImgFilename != filename) {
        config.wasDownloaded = false;
    }
    config.matchImgFilename = filename;
    config.filterCfg.roi_left = m_posXBox->value();
    config.filterCfg.roi_bottom = m_posYBox->value();
    config.filterCfg.per_pixel_err_thresh = float(m_perPixelErrorBox->value());
    config.totalMatchThresh = float(m_totalMatchThreshBox->value());
    config.invertResult = m_invertResultCheckbox->isChecked();

    config.maskMode = PmMaskMode(m_maskModeCombo->currentIndex());
    switch (config.maskMode) {
    case PmMaskMode::AlphaMode:
        config.filterCfg.mask_alpha = true;
        config.filterCfg.mask_color = { 0.f, 0.f, 0.f, 0.f };
        break;
    case PmMaskMode::BlackMode:
        config.filterCfg.mask_alpha = false;
        config.filterCfg.mask_color = { 0.f, 0.f, 0.f, 0.f };
        break;
    case PmMaskMode::GreenMode:
        config.filterCfg.mask_alpha = false;
        config.filterCfg.mask_color = { 0.f, 1.f, 0.f, 0.f };
        break;
    case PmMaskMode::MagentaMode:
        config.filterCfg.mask_alpha = false;
        config.filterCfg.mask_color = { 1.f, 0.f, 1.f, 0.f };
        break;
    case PmMaskMode::CustomClrMode:
        config.filterCfg.mask_alpha = false;
        config.filterCfg.mask_color = m_customColor;
        break;
    }

    maskModeChanged(config.maskMode, config.filterCfg.mask_color);
    emit sigMatchConfigChanged(m_matchIndex, config);
}
