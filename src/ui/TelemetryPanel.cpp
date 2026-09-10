#include "TelemetryPanel.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace {

void drawCard(QPainter& painter, const QRectF& rect,
              const QString& title, const QString& subtitle)
{
    painter.setPen(QPen(QColor(61, 139, 181, 120), 1.0));
    painter.setBrush(QColor(7, 29, 45, 172));
    painter.drawRoundedRect(rect, 7.0, 7.0);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(14);
    painter.setFont(font);
    painter.setPen(QColor(228, 244, 253));
    painter.drawText(rect.adjusted(10, 5, -10, -5),
                     Qt::AlignLeft | Qt::AlignTop, title);
    font.setBold(false);
    font.setPixelSize(10);
    painter.setFont(font);
    painter.setPen(QColor(130, 190, 218));
    painter.drawText(rect.adjusted(10, 23, -10, -5),
                     Qt::AlignLeft | Qt::AlignTop, subtitle);
}

double maximumForPlot(const QVector<double>& values, double floor)
{
    double maximum = floor;
    for (double value : values) maximum = qMax(maximum, value);
    return maximum;
}

} // namespace

TrendPlot::TrendPlot(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(125);
}

void TrendPlot::append(double activeVehicles, double queueVehicles,
                       double speedKph, double energyKWh)
{
    m_activeVehicles.push_back(activeVehicles);
    m_queueVehicles.push_back(queueVehicles);
    m_speed.push_back(speedKph);
    m_energy.push_back(energyKWh);
    const int maximum = 180;
    auto trim = [maximum](QVector<double>& values) {
        if (values.size() > maximum)
            values.remove(0, values.size() - maximum);
    };
    trim(m_activeVehicles);
    trim(m_queueVehicles);
    trim(m_speed);
    trim(m_energy);
    update();
}

void TrendPlot::clear()
{
    m_activeVehicles.clear();
    m_queueVehicles.clear();
    m_speed.clear();
    m_energy.clear();
    update();
}

void TrendPlot::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF card = QRectF(rect()).adjusted(1, 1, -1, -1);
    drawCard(painter, card,
             QStringLiteral("排队·速度·能耗趋势"),
             QStringLiteral("最近采样窗口；不同量纲分别归一化绘制"));
    const QRectF area = card.adjusted(12, 48, -12, -22);
    painter.setPen(QColor(255, 255, 255, 32));
    for (int i = 0; i <= 4; ++i) {
        const double y = area.top() + area.height() * i / 4.0;
        painter.drawLine(area.left(), y, area.right(), y);
    }
    auto drawSeries = [&](const QVector<double>& values, const QColor& color,
                          double minimumScale) {
        if (values.isEmpty()) return;
        const double scale = maximumForPlot(values, minimumScale);
        QPainterPath path;
        for (int i = 0; i < values.size(); ++i) {
            const double x = area.left()
                + area.width() * i / qMax(1, values.size() - 1);
            const double y = area.bottom()
                - area.height() * qBound(0.0, values[i] / scale, 1.0);
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        painter.setPen(QPen(color, 1.8));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    };
    drawSeries(m_activeVehicles, QColor("#5EA8FF"), 10.0);
    drawSeries(m_queueVehicles, QColor("#FFAA45"), 10.0);
    drawSeries(m_speed, QColor("#4DE2DA"), 30.0);
    drawSeries(m_energy, QColor("#C7A8FF"), 0.1);
    QFont legendFont = painter.font();
    legendFont.setPixelSize(10);
    legendFont.setBold(false);
    painter.setFont(legendFont);
    painter.setPen(QColor(199, 224, 239));
    painter.drawText(card.adjusted(12, 0, -12, -3),
                     Qt::AlignLeft | Qt::AlignBottom,
                     QStringLiteral("蓝 在网车辆   橙 排队   青 平均速度   紫 累计能耗"));
}

TrajectoryPlot::TrajectoryPlot(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(125);
}

void TrajectoryPlot::setTrajectories(const QVariantMap& trajectories)
{
    m_trajectories = trajectories;
    update();
}

void TrajectoryPlot::clear()
{
    m_trajectories.clear();
    update();
}

void TrajectoryPlot::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF card = QRectF(rect()).adjusted(1, 1, -1, -1);
    drawCard(painter, card,
             QStringLiteral("车辆轨迹采样"),
             QStringLiteral("记录位置、车道、速度；亮点为车辆当前位置"));
    const QRectF plot = card.adjusted(12, 48, -12, -12);

    bool hasPoint = false;
    double minX = 0.0, maxX = 0.0, minY = 0.0, maxY = 0.0;
    for (auto it = m_trajectories.cbegin(); it != m_trajectories.cend(); ++it) {
        for (const QVariant& value : it.value().toList()) {
            const QVariantMap point = value.toMap();
            const double x = point.value("xM").toDouble();
            const double y = point.value("yM").toDouble();
            if (!hasPoint) {
                minX = maxX = x;
                minY = maxY = y;
                hasPoint = true;
            } else {
                minX = qMin(minX, x);
                maxX = qMax(maxX, x);
                minY = qMin(minY, y);
                maxY = qMax(maxY, y);
            }
        }
    }
    if (!hasPoint) {
        painter.setPen(QColor(132, 176, 201));
        painter.drawText(plot, Qt::AlignCenter, QStringLiteral("等待仿真轨迹数据"));
        return;
    }
    if (maxX - minX < 1.0) { minX -= 0.5; maxX += 0.5; }
    if (maxY - minY < 1.0) { minY -= 0.5; maxY += 0.5; }
    auto mapPoint = [&](const QVariantMap& point) {
        const double x = (point.value("xM").toDouble() - minX) / (maxX - minX);
        const double y = (point.value("yM").toDouble() - minY) / (maxY - minY);
        return QPointF(plot.left() + x * plot.width(),
                       plot.bottom() - y * plot.height());
    };

    painter.setClipRect(plot);
    for (auto it = m_trajectories.cbegin(); it != m_trajectories.cend(); ++it) {
        const QVariantList trail = it.value().toList();
        if (trail.isEmpty()) continue;
        QPainterPath path;
        path.moveTo(mapPoint(trail.first().toMap()));
        for (int i = 1; i < trail.size(); ++i)
            path.lineTo(mapPoint(trail[i].toMap()));
        painter.setPen(QPen(QColor(52, 205, 208, 95), 1.15));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
        const QVariantMap last = trail.last().toMap();
        const double speed = last.value("speedKph").toDouble();
        const QColor color = speed < 5.0 ? QColor(255, 104, 92, 225)
            : speed < 20.0 ? QColor(255, 183, 71, 225)
                           : QColor(67, 224, 185, 225);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(mapPoint(last), 2.3, 2.3);
    }
}

TelemetryPanel::TelemetryPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("nativeTelemetryRoot"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumHeight(245);
    setStyleSheet(
        "QWidget#nativeTelemetryRoot{background:rgba(5,21,36,218);color:#e4f2fc;}"
        "QLabel{background:transparent;color:#d8edf8;}"
        "QFrame#metricCard{background:rgba(14,43,63,190);"
        "border:1px solid rgba(71,155,199,135);border-radius:6px;}"
        "QLabel#metricCaption{color:#8ec4e2;font-size:11px;}"
        "QLabel#metricValue{color:#f2faff;font-size:17px;font-weight:700;}"
        "QLabel#telemetrySubtitle{color:#50d6ca;font-size:11px;}"
        "QLabel#telemetryStatus{color:#8ec4e2;font-size:11px;}"
        "QPlainTextEdit{background:rgba(7,26,40,150);color:#cde5f4;"
        "border:1px solid rgba(71,155,199,100);border-radius:6px;"
        "font-size:12px;padding:3px;}");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(7, 5, 7, 6);
    root->setSpacing(5);

    auto* header = new QHBoxLayout();
    header->setSpacing(12);
    auto* title = new QLabel(QStringLiteral("数据采集与可视化"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(15);
    title->setFont(titleFont);
    auto* subtitle = new QLabel(
        QStringLiteral("TESSNG真实仿真数据 · 点击路网青色交叉口标记可查看单点详情"), this);
    subtitle->setObjectName(QStringLiteral("telemetrySubtitle"));
    m_statusLine = new QLabel(this);
    m_statusLine->setObjectName(QStringLiteral("telemetryStatus"));
    m_statusLine->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    header->addWidget(title);
    header->addWidget(subtitle);
    header->addStretch(1);
    header->addWidget(m_statusLine);
    root->addLayout(header);

    auto* metrics = new QGridLayout();
    metrics->setHorizontalSpacing(5);
    metrics->setVerticalSpacing(4);
    auto addMetric = [&](const QString& caption, int row, int column,
                         QLabel** valueLabel) {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("metricCard"));
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(7, 3, 7, 3);
        cardLayout->setSpacing(0);
        auto* captionLabel = new QLabel(caption, card);
        captionLabel->setObjectName(QStringLiteral("metricCaption"));
        auto* value = new QLabel(QStringLiteral("--"), card);
        value->setObjectName(QStringLiteral("metricValue"));
        cardLayout->addWidget(captionLabel);
        cardLayout->addWidget(value);
        metrics->addWidget(card, row, column);
        *valueLabel = value;
    };
    addMetric(QStringLiteral("正式仿真时间"), 0, 0, &m_time);
    addMetric(QStringLiteral("当前在网车辆"), 0, 1, &m_activeVehicles);
    addMetric(QStringLiteral("当前排队车辆"), 0, 2, &m_queue);
    addMetric(QStringLiteral("全网平均速度"), 0, 3, &m_speed);
    addMetric(QStringLiteral("累计净能耗"), 1, 0, &m_energy);
    addMetric(QStringLiteral("完成车辆平均延误"), 1, 1, &m_completedDelay);
    addMetric(QStringLiteral("实际完成车辆"), 1, 2, &m_completedVehicles);
    addMetric(QStringLiteral("实测仿真倍速"), 1, 3, &m_actualMultiple);
    for (int column = 0; column < 4; ++column)
        metrics->setColumnStretch(column, 1);
    root->addLayout(metrics);

    auto* content = new QHBoxLayout();
    content->setSpacing(5);
    m_trajectoryPlot = new TrajectoryPlot(this);
    m_plot = new TrendPlot(this);
    m_events = new QPlainTextEdit(this);
    m_events->setReadOnly(true);
    m_events->setMaximumBlockCount(300);
    m_events->setPlaceholderText(QStringLiteral("运行日志与诊断事件"));
    content->addWidget(m_trajectoryPlot, 5);
    content->addWidget(m_plot, 5);
    content->addWidget(m_events, 3);
    root->addLayout(content, 1);
    resetView();
}

void TelemetryPanel::setMetricColor(QLabel* label, const QString& color)
{
    if (label) label->setStyleSheet(QStringLiteral("color:%1;").arg(color));
}

void TelemetryPanel::updateTelemetry(const QVariantMap& value)
{
    const double speed = value.value("speed").toDouble();
    const int queue = value.value("queue").toInt();
    const int running = value.value("running").toInt();
    const double energy = value.value("energy").toDouble();
    const double actualMultiple = value.value("multiple").toDouble();
    const int targetMultiple = value.value("targetMultiple", 1).toInt();
    const int schedulerMultiple = value.value(
        "schedulerMultiple", targetMultiple).toInt();
    const bool speedCapacityLimited =
        value.value("speedCapacityLimited").toBool();
    const int delaySamples = value.value("completedDelaySamples").toInt();
    const double completedDelaySec = value.value("completedDelaySec").toDouble();
    const int actualCompletedVehicles =
        value.value("actualCompletedVehicles").toInt();

    m_time->setText(QStringLiteral("%1 s")
        .arg(value.value("time").toDouble(), 0, 'f', 1));
    m_activeVehicles->setText(QStringLiteral("%1 辆").arg(running));
    m_queue->setText(QStringLiteral("%1 辆").arg(queue));
    m_speed->setText(QStringLiteral("%1 km/h").arg(speed, 0, 'f', 1));
    m_energy->setText(QStringLiteral("%1 kWh").arg(energy, 0, 'f', 3));
    m_completedDelay->setText(delaySamples > 0 && completedDelaySec >= 0.0
        ? QStringLiteral("%1 s").arg(completedDelaySec, 0, 'f', 1)
        : QStringLiteral("--"));
    m_completedVehicles->setText(
        QStringLiteral("%1 辆").arg(actualCompletedVehicles));
    m_completedVehicles->setToolTip(QStringLiteral(
        "仅统计正式仿真阶段确实到达固定OD目标终点的车辆；预加载车辆、错误出口和异常消失均不计入。"));
    m_actualMultiple->setText(actualMultiple > 0.0
        ? QStringLiteral("%1× / 目标%2×%3")
            .arg(actualMultiple, 0, 'f', 2)
            .arg(targetMultiple)
            .arg(speedCapacityLimited ? QStringLiteral(" · 性能上限") : QString())
        : QStringLiteral("测速中 / 目标%1×").arg(targetMultiple));
    m_actualMultiple->setToolTip(QStringLiteral(
        "实测值=仿真时间增量÷墙钟时间增量；当前TESSNG内部调度命令为%1×。")
        .arg(schedulerMultiple));
    setMetricColor(m_time, QStringLiteral("#67E8F9"));
    setMetricColor(m_activeVehicles, QStringLiteral("#7DD3FC"));
    setMetricColor(m_queue, queue >= 80 ? QStringLiteral("#FF7B72")
        : queue >= 20 ? QStringLiteral("#FDBA74") : QStringLiteral("#6EE7B7"));
    setMetricColor(m_speed, speed < 10.0 ? QStringLiteral("#FF7B72")
        : speed < 25.0 ? QStringLiteral("#FDBA74") : QStringLiteral("#6EE7B7"));
    setMetricColor(m_energy, QStringLiteral("#FDE68A"));
    setMetricColor(m_completedDelay, QStringLiteral("#C4B5FD"));
    setMetricColor(m_completedVehicles, QStringLiteral("#6EE7B7"));
    setMetricColor(m_actualMultiple, speedCapacityLimited
        ? QStringLiteral("#FF7B72")
        : actualMultiple > 0.0 && actualMultiple < targetMultiple * 0.85
            ? QStringLiteral("#FDBA74") : QStringLiteral("#D8B4FE"));
    m_statusLine->setText(QStringLiteral("%1 | %2 | RSU: %3 | 采样: %4次 / 轨迹: %5辆")
        .arg(value.value("runState").toString(),
             value.value("controlMode").toString(),
             value.value("rsuMode").toString())
        .arg(value.value("sampleCount").toLongLong())
        .arg(value.value("trajectoryVehicleCount").toInt()));
    m_plot->append(running, queue, speed, energy);
    m_trajectoryPlot->setTrajectories(value.value("trajectories").toMap());
}

void TelemetryPanel::appendEvent(const QString& text)
{
    m_events->appendPlainText(text);
}

void TelemetryPanel::resetView()
{
    m_plot->clear();
    m_trajectoryPlot->clear();
    m_time->setText(QStringLiteral("0.0 s"));
    m_activeVehicles->setText(QStringLiteral("0 辆"));
    m_queue->setText(QStringLiteral("0 辆"));
    m_speed->setText(QStringLiteral("0.0 km/h"));
    m_energy->setText(QStringLiteral("0.000 kWh"));
    m_completedDelay->setText(QStringLiteral("--"));
    m_completedVehicles->setText(QStringLiteral("0 辆"));
    m_actualMultiple->setText(QStringLiteral("等待测速"));
    m_statusLine->setText(QStringLiteral(
        "等待手动开始 | IMPROVED_MAX_PRESSURE | RSU: VIRTUAL_RSU_TESSNG | 采样: 0次 / 轨迹: 0辆"));
    setMetricColor(m_time, QStringLiteral("#67E8F9"));
    setMetricColor(m_activeVehicles, QStringLiteral("#7DD3FC"));
    setMetricColor(m_queue, QStringLiteral("#6EE7B7"));
    setMetricColor(m_speed, QStringLiteral("#6EE7B7"));
    setMetricColor(m_energy, QStringLiteral("#FDE68A"));
    setMetricColor(m_completedDelay, QStringLiteral("#C4B5FD"));
    setMetricColor(m_completedVehicles, QStringLiteral("#6EE7B7"));
    setMetricColor(m_actualMultiple, QStringLiteral("#D8B4FE"));
    m_events->clear();
}
