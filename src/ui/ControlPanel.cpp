#include "ControlPanel.h"

#include <QButtonGroup>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QLocale>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QLabel* makeNote(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("controlNote"));
    label->setWordWrap(true);
    return label;
}

QPushButton* makeWideButton(const QString& text, QWidget* parent)
{
    auto* button = new QPushButton(text, parent);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return button;
}

} // namespace

ControlPanel::ControlPanel(int vehicleSeed,
                           int destinationSeed,
                           int preloadCount,
                           int preloadMaximum,
                           int durationSec,
                           int outerRate,
                           int residentialRate,
                           int outerEntryCount,
                           int residentialEntryCount,
                           int comparisonRounds,
                           const QString& initialAlgorithm,
                           QWidget* parent)
    : QWidget(parent),
      m_outerEntryCount(qMax(0, outerEntryCount)),
      m_residentialEntryCount(qMax(0, residentialEntryCount))
{
    setObjectName(QStringLiteral("nativeControlRoot"));
    setMinimumWidth(275);
    setMaximumWidth(355);
    setStyleSheet(
        "QWidget#nativeControlRoot{background:#f4f4f4;color:#17212b;}"
        "QGroupBox{font-size:14px;font-weight:600;border:1px solid #d4d7dc;"
        "border-radius:2px;margin-top:10px;padding-top:8px;}"
        "QGroupBox::title{subcontrol-origin:margin;left:8px;padding:0 3px;}"
        "QPushButton{min-height:27px;font-size:14px;padding:1px 6px;}"
        "QPushButton:checked{background:#cfe8ff;border:1px solid #1775c9;"
        "border-radius:3px;color:#064c87;}"
        "QSpinBox{min-height:26px;font-size:14px;background:white;}"
        "QLabel{font-size:14px;background:transparent;}"
        "QLabel#controlNote{font-size:12px;color:#4b5563;}"
        "QLabel#subtotal{font-size:13px;color:#374151;font-weight:600;}"
        "QLabel#total{font-size:13px;color:#0b5ea8;font-weight:700;}"
        "QScrollArea{border:0;background:#f4f4f4;}");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("nativeControlRoot"));
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(7, 7, 7, 8);
    layout->setSpacing(5);

    auto* title = new QLabel(QStringLiteral("雄安窄密路网仿真控制"), content);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(15);
    title->setFont(titleFont);
    layout->addWidget(title);

    auto* runningGroup = new QGroupBox(QStringLiteral("仿真运行"), content);
    auto* runningLayout = new QGridLayout(runningGroup);
    runningLayout->setContentsMargins(7, 10, 7, 7);
    runningLayout->setSpacing(5);
    auto* start = makeWideButton(QStringLiteral("开始 / 继续"), runningGroup);
    auto* pause = makeWideButton(QStringLiteral("暂停"), runningGroup);
    m_reset = makeWideButton(QStringLiteral("重置本轮（可换算法）"), runningGroup);
    auto* stop = makeWideButton(QStringLiteral("完全终止"), runningGroup);
    stop->setObjectName(QStringLiteral("stopButton"));
    stop->setStyleSheet("QPushButton{color:#d7191c;font-weight:700;}");
    runningLayout->addWidget(start, 0, 0);
    runningLayout->addWidget(pause, 0, 1);
    runningLayout->addWidget(m_reset, 1, 0, 1, 2);
    runningLayout->addWidget(stop, 2, 0, 1, 2);
    runningLayout->addWidget(new QLabel(QStringLiteral("目标倍速"), runningGroup), 3, 0);
    m_speed = new QSpinBox(runningGroup);
    m_speed->setRange(1, 50);
    m_speed->setValue(1);
    m_speed->setSuffix(QStringLiteral(" ×"));
    runningLayout->addWidget(m_speed, 3, 1);
    runningLayout->addWidget(new QLabel(QStringLiteral("仿真时长"), runningGroup), 4, 0);
    auto* durationRow = new QWidget(runningGroup);
    auto* durationLayout = new QHBoxLayout(durationRow);
    durationLayout->setContentsMargins(0, 0, 0, 0);
    durationLayout->setSpacing(5);
    m_duration = new QSpinBox(durationRow);
    m_duration->setRange(60, 86400);
    m_duration->setSingleStep(60);
    m_duration->setValue(qBound(60, durationSec, 86400));
    m_duration->setSuffix(QStringLiteral(" s"));
    auto* applyDuration = new QPushButton(QStringLiteral("应用时长"), durationRow);
    durationLayout->addWidget(m_duration, 1);
    durationLayout->addWidget(applyDuration);
    runningLayout->addWidget(durationRow, 4, 1);
    runningLayout->addWidget(
        makeNote(QStringLiteral("使用配置文件中的计划仿真时长"), runningGroup),
        5, 0, 1, 2);
    layout->addWidget(runningGroup);

    auto* algorithmGroup = new QGroupBox(QStringLiteral("控制算法（下一轮生效）"), content);
    auto* algorithmLayout = new QVBoxLayout(algorithmGroup);
    algorithmLayout->setContentsMargins(7, 10, 7, 7);
    algorithmLayout->setSpacing(5);
    m_fixedTiming = makeWideButton(QStringLiteral("固定配时（实验基准）"), algorithmGroup);
    m_maxPressure = makeWideButton(QStringLiteral("改进最大压力（默认）"), algorithmGroup);
    m_fixedTiming->setCheckable(true);
    m_maxPressure->setCheckable(true);
    auto* algorithmButtons = new QButtonGroup(algorithmGroup);
    algorithmButtons->setExclusive(true);
    algorithmButtons->addButton(m_fixedTiming);
    algorithmButtons->addButton(m_maxPressure);
    algorithmLayout->addWidget(m_fixedTiming);
    algorithmLayout->addWidget(m_maxPressure);
    m_algorithmHint = makeNote(
        QStringLiteral("当前使用配置文件中的默认控制算法"), algorithmGroup);
    algorithmLayout->addWidget(m_algorithmHint);
    layout->addWidget(algorithmGroup);
    selectAlgorithm(initialAlgorithm);

    auto* comparisonGroup = new QGroupBox(QStringLiteral("多轮对照仿真"), content);
    auto* comparisonLayout = new QGridLayout(comparisonGroup);
    comparisonLayout->setContentsMargins(7, 10, 7, 7);
    comparisonLayout->setSpacing(5);
    comparisonLayout->addWidget(new QLabel(QStringLiteral("对照轮次"), comparisonGroup), 0, 0);
    m_comparisonRounds = new QSpinBox(comparisonGroup);
    m_comparisonRounds->setRange(1, 100);
    m_comparisonRounds->setValue(qBound(1, comparisonRounds, 100));
    m_comparisonRounds->setSuffix(QStringLiteral(" 轮"));
    comparisonLayout->addWidget(m_comparisonRounds, 0, 1);
    m_comparisonStart = new QPushButton(QStringLiteral("开始自动对照"), comparisonGroup);
    m_comparisonCancel = new QPushButton(QStringLiteral("停止自动对照"), comparisonGroup);
    m_comparisonCancel->setEnabled(false);
    comparisonLayout->addWidget(m_comparisonStart, 1, 0);
    comparisonLayout->addWidget(m_comparisonCancel, 1, 1);
    m_comparisonHint = makeNote(
        QStringLiteral("1轮 = 改进最大压力 + 固定配时；同轮共用种子"), comparisonGroup);
    comparisonLayout->addWidget(m_comparisonHint, 2, 0, 1, 2);
    layout->addWidget(comparisonGroup);

    auto* seedGroup = new QGroupBox(QStringLiteral("实验随机种子（下一轮生效）"), content);
    auto* seedLayout = new QGridLayout(seedGroup);
    seedLayout->setContentsMargins(7, 10, 7, 7);
    seedLayout->setSpacing(5);
    seedLayout->addWidget(new QLabel(QStringLiteral("车辆产生（随机）"), seedGroup), 0, 0);
    m_vehicleSeed = new QSpinBox(seedGroup);
    m_vehicleSeed->setRange(0, 2147483647);
    m_vehicleSeed->setValue(qBound(0, vehicleSeed, 2147483647));
    m_vehicleSeed->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_vehicleSeed->setReadOnly(false);
    m_vehicleSeed->setFocusPolicy(Qt::StrongFocus);
    m_vehicleSeed->setToolTip(QStringLiteral("可直接输入整数，点击“应用到下一轮”后生效"));
    seedLayout->addWidget(m_vehicleSeed, 0, 1);
    seedLayout->addWidget(new QLabel(QStringLiteral("目的地选择（随机）"), seedGroup), 1, 0);
    m_destinationSeed = new QSpinBox(seedGroup);
    m_destinationSeed->setRange(0, 2147483647);
    m_destinationSeed->setValue(qBound(0, destinationSeed, 2147483647));
    m_destinationSeed->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_destinationSeed->setReadOnly(false);
    m_destinationSeed->setFocusPolicy(Qt::StrongFocus);
    m_destinationSeed->setToolTip(QStringLiteral("可直接输入整数，点击“应用到下一轮”后生效"));
    seedLayout->addWidget(m_destinationSeed, 1, 1);
    auto* randomizeSeeds = new QPushButton(QStringLiteral("随机生成"), seedGroup);
    auto* applySeeds = new QPushButton(QStringLiteral("应用到下一轮"), seedGroup);
    seedLayout->addWidget(randomizeSeeds, 2, 0);
    seedLayout->addWidget(applySeeds, 2, 1);
    m_seedHint = makeNote(QStringLiteral("使用配置文件中的两项随机种子"), seedGroup);
    seedLayout->addWidget(m_seedHint, 3, 0, 1, 2);
    layout->addWidget(seedGroup);

    auto* demandGroup = new QGroupBox(QStringLiteral("分组车辆发生量"), content);
    auto* demandLayout = new QVBoxLayout(demandGroup);
    demandLayout->setContentsMargins(7, 10, 7, 7);
    demandLayout->setSpacing(4);
    demandLayout->addWidget(new QLabel(
        QStringLiteral("启用的外围端点（%1个、每点独立流率）")
            .arg(m_outerEntryCount), demandGroup));
    auto* outerRow = new QHBoxLayout();
    m_outerRate = new QSpinBox(demandGroup);
    m_outerRate->setRange(0, 5000);
    m_outerRate->setSingleStep(50);
    m_outerRate->setValue(qMax(0, outerRate));
    m_outerRate->setSuffix(QStringLiteral(" 辆/h"));
    auto* applyOuter = new QPushButton(
        QStringLiteral("应用到外围%1点").arg(m_outerEntryCount), demandGroup);
    outerRow->addWidget(m_outerRate, 1);
    outerRow->addWidget(applyOuter);
    demandLayout->addLayout(outerRow);
    m_outerSubtotal = new QLabel(demandGroup);
    m_outerSubtotal->setObjectName(QStringLiteral("subtotal"));
    demandLayout->addWidget(m_outerSubtotal);
    demandLayout->addWidget(new QLabel(
        QStringLiteral("启用的居民区端点（%1个、每点独立流率）")
            .arg(m_residentialEntryCount), demandGroup));
    auto* residentialRow = new QHBoxLayout();
    m_residentialRate = new QSpinBox(demandGroup);
    m_residentialRate->setRange(0, 5000);
    m_residentialRate->setSingleStep(50);
    m_residentialRate->setValue(qMax(0, residentialRate));
    m_residentialRate->setSuffix(QStringLiteral(" 辆/h"));
    auto* applyResidential = new QPushButton(
        QStringLiteral("应用到居民区%1点").arg(m_residentialEntryCount), demandGroup);
    residentialRow->addWidget(m_residentialRate, 1);
    residentialRow->addWidget(applyResidential);
    demandLayout->addLayout(residentialRow);
    m_residentialSubtotal = new QLabel(demandGroup);
    m_residentialSubtotal->setObjectName(QStringLiteral("subtotal"));
    demandLayout->addWidget(m_residentialSubtotal);
    m_totalDemand = new QLabel(demandGroup);
    m_totalDemand->setObjectName(QStringLiteral("total"));
    demandLayout->addWidget(m_totalDemand);
    demandLayout->addWidget(
        makeNote(QStringLiteral("使用当前路网中的分组发车计划"), demandGroup));
    layout->addWidget(demandGroup);

    auto* preloadGroup = new QGroupBox(QStringLiteral("预加载车辆（下一轮生效）"), content);
    auto* preloadLayout = new QHBoxLayout(preloadGroup);
    preloadLayout->setContentsMargins(7, 10, 7, 7);
    m_preloadCount = new QSpinBox(preloadGroup);
    m_preloadCount->setRange(0, qMax(0, preloadMaximum));
    m_preloadCount->setValue(qBound(0, preloadCount, qMax(0, preloadMaximum)));
    m_preloadCount->setSuffix(QStringLiteral(" 辆"));
    auto* applyPreload = new QPushButton(QStringLiteral("应用到下一轮"), preloadGroup);
    preloadLayout->addWidget(m_preloadCount, 1);
    preloadLayout->addWidget(applyPreload);
    layout->addWidget(preloadGroup);
    layout->addStretch(1);
    scroll->setWidget(content);
    root->addWidget(scroll);

    // Every numeric field accepts keyboard input.  Values with an adjacent
    // “apply” button are committed only when that button is clicked.
    const QList<QSpinBox*> editableNumbers = {
        m_speed, m_duration, m_vehicleSeed, m_destinationSeed,
        m_outerRate, m_residentialRate, m_preloadCount, m_comparisonRounds
    };
    for (QSpinBox* spinBox : editableNumbers) {
        spinBox->setReadOnly(false);
        spinBox->setFocusPolicy(Qt::StrongFocus);
        spinBox->setKeyboardTracking(false);
    }

    refreshDemandPreview();

    connect(start, &QPushButton::clicked, this, &ControlPanel::startRequested);
    connect(pause, &QPushButton::clicked, this, &ControlPanel::pauseRequested);
    connect(m_reset, &QPushButton::clicked, this, &ControlPanel::resetRequested);
    connect(stop, &QPushButton::clicked, this, &ControlPanel::stopRequested);
    connect(m_speed, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ControlPanel::speedChanged);
    connect(applyDuration, &QPushButton::clicked, this, [this]() {
        emit durationChanged(m_duration->value());
    });
    connect(m_fixedTiming, &QPushButton::clicked, this, [this]() {
        selectAlgorithm(QStringLiteral("FIXED_TIMING"));
        emit algorithmChanged(QStringLiteral("FIXED_TIMING"));
    });
    connect(m_maxPressure, &QPushButton::clicked, this, [this]() {
        selectAlgorithm(QStringLiteral("IMPROVED_MAX_PRESSURE"));
        emit algorithmChanged(QStringLiteral("IMPROVED_MAX_PRESSURE"));
    });
    connect(randomizeSeeds, &QPushButton::clicked, this, [this]() {
        m_vehicleSeed->setValue(static_cast<int>(
            QRandomGenerator::global()->generate() & 0x7fffffffu));
        m_destinationSeed->setValue(static_cast<int>(
            QRandomGenerator::global()->generate() & 0x7fffffffu));
        m_seedHint->setText(QStringLiteral("已随机生成，点击“应用到下一轮”后生效"));
    });
    connect(applySeeds, &QPushButton::clicked, this, [this]() {
        emit randomSeedsChanged(m_vehicleSeed->value(), m_destinationSeed->value());
        m_seedHint->setText(QStringLiteral("两项随机种子已应用到下一轮"));
    });
    connect(m_outerRate, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { refreshDemandPreview(); });
    connect(m_residentialRate, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { refreshDemandPreview(); });
    connect(applyOuter, &QPushButton::clicked, this, [this]() {
        emit outerRateChanged(m_outerRate->value());
    });
    connect(applyResidential, &QPushButton::clicked, this, [this]() {
        emit residentialRateChanged(m_residentialRate->value());
    });
    connect(applyPreload, &QPushButton::clicked, this, [this]() {
        emit preloadCountChanged(m_preloadCount->value());
    });
    connect(m_comparisonStart, &QPushButton::clicked, this, [this]() {
        emit comparisonRequested(m_comparisonRounds->value());
    });
    connect(m_comparisonCancel, &QPushButton::clicked,
            this, &ControlPanel::comparisonCancelRequested);

    m_comparisonLockedWidgets = {
        algorithmGroup, seedGroup, demandGroup, preloadGroup,
        m_duration, applyDuration, m_reset
    };
}

void ControlPanel::updateComparisonState(bool active,
                                         int currentRound,
                                         int totalRounds,
                                         const QString& algorithm,
                                         int vehicleSeed,
                                         int destinationSeed,
                                         int resultRound,
                                         const QString& status)
{
    m_comparisonRounds->setEnabled(!active);
    m_comparisonStart->setEnabled(!active);
    m_comparisonCancel->setEnabled(active);
    for (QWidget* widget : m_comparisonLockedWidgets)
        if (widget) widget->setEnabled(!active);
    if (!active) {
        m_comparisonHint->setText(status.isEmpty()
            ? QStringLiteral("1轮 = 改进最大压力 + 固定配时；同轮共用种子")
            : status);
        return;
    }
    if (currentRound <= 0 || resultRound <= 0) {
        m_comparisonHint->setText(status);
        return;
    }
    const QString algorithmText = algorithm == QStringLiteral("FIXED_TIMING")
        ? QStringLiteral("固定配时") : QStringLiteral("改进最大压力");
    m_comparisonHint->setText(QStringLiteral(
        "%1｜第%2/%3轮｜%4｜结果round_%5｜种子%6/%7")
        .arg(status).arg(currentRound).arg(totalRounds).arg(algorithmText)
        .arg(resultRound, 3, 10, QChar('0')).arg(vehicleSeed).arg(destinationSeed));
}

void ControlPanel::refreshDemandPreview()
{
    const QLocale locale;
    const int outerTotal = m_outerEntryCount * m_outerRate->value();
    const int residentialTotal = m_residentialEntryCount * m_residentialRate->value();
    m_outerSubtotal->setText(QStringLiteral("外围小计：%1 辆/h（%2点 × %3）")
        .arg(locale.toString(outerTotal))
        .arg(m_outerEntryCount)
        .arg(locale.toString(m_outerRate->value())));
    m_residentialSubtotal->setText(QStringLiteral("居民区小计：%1 辆/h（%2点 × %3）")
        .arg(locale.toString(residentialTotal))
        .arg(m_residentialEntryCount)
        .arg(locale.toString(m_residentialRate->value())));
    m_totalDemand->setText(QStringLiteral("%1个启用入口合计目标：%2 辆/h")
        .arg(m_outerEntryCount + m_residentialEntryCount)
        .arg(locale.toString(outerTotal + residentialTotal)));
}

void ControlPanel::selectAlgorithm(const QString& algorithm)
{
    const bool fixed = algorithm.compare(QStringLiteral("FIXED_TIMING"),
                                         Qt::CaseInsensitive) == 0;
    m_fixedTiming->setChecked(fixed);
    m_maxPressure->setChecked(!fixed);
    m_algorithmHint->setText(fixed
        ? QStringLiteral("已选择固定配时；重置后下一轮生效")
        : QStringLiteral("已选择改进最大压力；重置后下一轮生效"));
}
