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
#include <QFileDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>

#include <obs-module.h>

PmDialog::PmDialog(PmCore *pixelMatcher, QWidget *parent)
: QDialog(parent)
, m_core(pixelMatcher)
, m_prevResults(pixelMatcher->results())
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));

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

    // match location
    QHBoxLayout *matchLocSubLayout = new QHBoxLayout;
    matchLocSubLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *posXLabel = new QLabel("X = ", this);
    matchLocSubLayout->addWidget(posXLabel);
    m_posXBox = new QSpinBox(this);
    m_posXBox->setSuffix(" px");
    m_posXBox->setRange(0, 0);
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
    m_posYBox->setRange(0, 0);
    m_posYBox->setSingleStep(1);
    m_posYBox->setValue(config.roiBottom);
    connect(m_posYBox, SIGNAL(valueChanged(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    matchLocSubLayout->addWidget(m_posYBox);

    mainTabLayout->addRow(obs_module_text("Match Location: "), matchLocSubLayout);

    // total match error threshold
    m_totalMatchThreshBox = new QDoubleSpinBox(this);
    m_totalMatchThreshBox->setSuffix(" %");
    m_totalMatchThreshBox->setRange(0.0, 100.0);
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

    // image/match display area
    m_filterDisplay = new OBSQTDisplay(this);
    m_filterDisplay->setSizePolicy(
        QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_filterDisplay->setMinimumSize(400, 300);
    m_filterDisplay->setMaximumSize(4000, 3000);
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

    // signals & slots
    connect(m_core, &PmCore::sigImgSuccess,
            this, &PmDialog::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
            this, &PmDialog::onImgFailed, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewResults,
            this, &PmDialog::onNewResults, Qt::QueuedConnection);
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

void PmDialog::colorModeChanged(PmDialog::ColorMode mode, QColor color)
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
    colorModeChanged(mode, QColor());
}

void PmDialog::onBrowseButtonReleased()
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

void PmDialog::onImgSuccess(QString filename)
{
    m_imgPathEdit->setText(filename);
    m_imgPathEdit->setStyleSheet("");
}

void PmDialog::onImgFailed(QString filename)
{
    m_imgPathEdit->setText(
        filename.size() ? QString("[FAILED] %1").arg(filename) : "");
    m_imgPathEdit->setStyleSheet("text-color: red");
}

void PmDialog::onNewResults(PmResultsPacket results)
{
    if (m_prevResults.baseWidth != results.baseWidth
     || m_prevResults.matchImgWidth != results.matchImgWidth) {
        m_posXBox->setMaximum(int(results.baseWidth - results.matchImgWidth));
    }
    if (m_prevResults.baseHeight != results.baseHeight
     || m_prevResults.matchImgHeight != results.matchImgHeight) {
        m_posYBox->setMaximum(int(results.baseHeight - results.matchImgHeight));
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

void PmDialog::onConfigUiChanged()
{
    PmConfigPacket config;
    config.roiLeft = m_posXBox->value();
    config.roiBottom = m_posYBox->value();
    config.totalMatchThresh = float(m_totalMatchThreshBox->value());
    emit sigNewUiConfig(config);
}
