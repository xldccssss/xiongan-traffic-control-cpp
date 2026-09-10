#include "IntersectionDetailPanel.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace {

void drawArrow(QPainter& painter, const QPointF& from, const QPointF& to,
               const QColor& color, double progress)
{
    painter.setPen(QPen(color, 2.2));
    painter.drawLine(from, to);
    const QPointF direction = to - from;
    const double length = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    if (length < 1.0) return;
    const QPointF unit = direction / length;
    const QPointF normal(-unit.y(), unit.x());
    QPolygonF arrow;
    arrow << to << to - unit * 9.0 + normal * 4.5 << to - unit * 9.0 - normal * 4.5;
    painter.setBrush(color);
    painter.drawPolygon(arrow);
    const QPointF pulse = from + direction * progress;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 235));
    painter.drawEllipse(pulse, 4.0, 4.0);
}

void drawNode(QPainter& painter, const QRectF& rect, const QColor& color,
              const QString& title, const QString& detail)
{
    painter.setPen(QPen(QColor(color.red(), color.green(), color.blue(), 210), 1.5));
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 42));
    painter.drawRoundedRect(rect, 8.0, 8.0);
    QFont font = painter.font();
    font.setPixelSize(13);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(color.lighter(125));
    painter.drawText(rect.adjusted(6, 8, -6, -6), Qt::AlignHCenter | Qt::AlignTop, title);
    font.setPixelSize(9);
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(QColor(205, 229, 244));
    painter.drawText(rect.adjusted(5, 30, -5, -5), Qt::AlignCenter | Qt::TextWordWrap, detail);
}

QLabel* makeValueCard(const QString& caption, QGridLayout* grid, int row, int column,
                      QWidget* parent)
{
    auto* box = new QWidget(parent);
    box->setProperty("metricCard", true);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(8, 5, 8, 5);
    layout->setSpacing(1);
    auto* name = new QLabel(caption, box);
    name->setProperty("caption", true);
    auto* value = new QLabel(QStringLiteral("--"), box);
    value->setProperty("value", true);
    layout->addWidget(name);
    layout->addWidget(value);
    grid->addWidget(box, row, column);
    return value;
}

} // namespace

DataFlowCanvas::DataFlowCanvas(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(205);
    m_animation = new QTimer(this);
    m_animation->setInterval(100);
    connect(m_animation, &QTimer::timeout, this, [this]() {
        m_progress += 0.035;
        if (m_progress > 1.0) m_progress -= 1.0;
        update();
    });
    m_animation->start();
}

void DataFlowCanvas::setSnapshot(const QVariantMap& values)
{
    const double queue = values.value("intersectionQueue").toDouble();
    const double speed = values.value("intersectionSpeed").toDouble();
    if (m_hasPrevious) {
        m_queueDelta = queue - m_previousQueue;
        m_speedDelta = speed - m_previousSpeed;
    }
    m_previousQueue = queue;
    m_previousSpeed = speed;
    m_hasPrevious = true;
    m_values = values;
    update();
}

void DataFlowCanvas::clear()
{
    m_values.clear();
    m_hasPrevious = false;
    m_queueDelta = m_speedDelta = 0.0;
    update();
}

void DataFlowCanvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF outer = QRectF(rect()).adjusted(1, 1, -1, -1);
    painter.setPen(QPen(QColor(71, 155, 199, 115), 1.0));
    painter.setBrush(QColor(7, 29, 45, 155));
    painter.drawRoundedRect(outer, 8.0, 8.0);
    painter.setPen(QColor(228, 244, 253));
    QFont title = painter.font();
    title.setBold(true); title.setPixelSize(13); painter.setFont(title);
    painter.drawText(outer.adjusted(10, 6, -10, -5), Qt::AlignLeft | Qt::AlignTop,
                     QStringLiteral("车—路—边—云动态数据流（亮点表示数据正在传输）"));

    const double gap = 16.0;
    const double nodeWidth = (outer.width() - 5.0 * gap) / 4.0;
    const double top = outer.top() + 42.0;
    const double nodeHeight = 92.0;
    QVector<QRectF> nodes;
    for (int i = 0; i < 4; ++i)
        nodes.push_back(QRectF(outer.left() + gap + i * (nodeWidth + gap),
                               top, nodeWidth, nodeHeight));

    drawNode(painter, nodes[0], QColor("#55A8FF"), QStringLiteral("车端 CAV"),
             QStringLiteral("位置 / 速度 / 轨迹\n车辆状态反馈"));
    const bool mergedIntersection = m_values.value("mergedIntersection").toBool();
    drawNode(painter, nodes[1], QColor("#35D2C6"), QStringLiteral("路侧 RSU"),
             mergedIntersection
                 ? QStringLiteral("J253+J254外边界采集\n排队 / 占有率 / 到达率")
                 : QStringLiteral("排队 / 占有率 / 到达率\n本地冲突检测"));
    drawNode(painter, nodes[2], QColor("#FFB547"), QStringLiteral("动态主从"),
             QStringLiteral("租约 / 主从切换\n边云提案融合"));
    drawNode(painter, nodes[3], QColor("#B485FF"), QStringLiteral("云端调度"),
             QStringLiteral("压力 / 配时 / 路由代价\n全局协同决策"));

    for (int i = 0; i < 3; ++i)
        drawArrow(painter, QPointF(nodes[i].right() + 2, nodes[i].center().y()),
                  QPointF(nodes[i + 1].left() - 2, nodes[i + 1].center().y()),
                  i == 0 ? QColor("#35D2C6") : i == 1 ? QColor("#FFB547")
                                                         : QColor("#B485FF"),
                  qBound(0.0, m_progress + i * 0.23, 1.0));

    const double bottomY = outer.bottom() - 24.0;
    drawArrow(painter, QPointF(nodes[3].center().x(), nodes[3].bottom() + 5),
              QPointF(nodes[0].center().x(), nodes[0].bottom() + 5),
              QColor("#FF6F91"), 1.0 - m_progress);
    painter.setPen(QColor(255, 188, 207));
    QFont small = painter.font(); small.setPixelSize(9); small.setBold(false); painter.setFont(small);
    painter.drawText(QRectF(nodes[0].center().x(), bottomY - 8,
                            nodes[3].center().x() - nodes[0].center().x(), 18),
                     Qt::AlignCenter,
                     mergedIntersection
                         ? QStringLiteral("控制回传：合并相位 / 单一灯色 / 速度建议 / 下一跳路径")
                         : QStringLiteral("控制回传：相位 / 灯色 / 速度建议 / 下一跳路径"));

    painter.setPen(QColor(255, 218, 114));
    QString owner = m_values.value("owner").toString();
    if (owner.isEmpty()) owner = QStringLiteral("--");
    painter.drawText(outer.adjusted(12, 0, -12, -4), Qt::AlignLeft | Qt::AlignBottom,
        QStringLiteral("本周期变化：排队 %1%2 辆   平均速度 %3%4 km/h   当前主控：%5")
            .arg(m_queueDelta >= 0 ? "+" : "").arg(m_queueDelta, 0, 'f', 1)
            .arg(m_speedDelta >= 0 ? "+" : "").arg(m_speedDelta, 0, 'f', 1)
            .arg(owner));
}

IntersectionTrendPlot::IntersectionTrendPlot(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(130);
}

void IntersectionTrendPlot::append(double queue, double speed, double arrivals)
{
    m_queue.push_back(queue); m_speed.push_back(speed); m_arrivals.push_back(arrivals);
    const int maximum = 120;
    if (m_queue.size() > maximum) {
        m_queue.remove(0, m_queue.size() - maximum);
        m_speed.remove(0, m_speed.size() - maximum);
        m_arrivals.remove(0, m_arrivals.size() - maximum);
    }
    update();
}

void IntersectionTrendPlot::clear()
{
    m_queue.clear(); m_speed.clear(); m_arrivals.clear(); update();
}

void IntersectionTrendPlot::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF outer = QRectF(rect()).adjusted(1, 1, -1, -1);
    painter.setPen(QPen(QColor(71, 155, 199, 105), 1.0));
    painter.setBrush(QColor(7, 29, 45, 145));
    painter.drawRoundedRect(outer, 7, 7);
    painter.setPen(QColor(220, 240, 250));
    painter.drawText(outer.adjusted(10, 5, -10, -5), Qt::AlignLeft | Qt::AlignTop,
                     QStringLiteral("单路口数据变化：橙=排队  青=速度  紫=到达率"));
    const QRectF plot = outer.adjusted(10, 27, -10, -10);
    auto draw = [&](const QVector<double>& values, const QColor& color) {
        if (values.size() < 2) return;
        double maximum = 1.0;
        for (double value : values) maximum = qMax(maximum, value);
        QPainterPath path;
        for (int i = 0; i < values.size(); ++i) {
            const QPointF p(plot.left() + plot.width() * i / qMax(1, values.size() - 1),
                            plot.bottom() - plot.height() * values[i] / maximum);
            if (i == 0) path.moveTo(p); else path.lineTo(p);
        }
        painter.setPen(QPen(color, 1.8)); painter.setBrush(Qt::NoBrush); painter.drawPath(path);
    };
    draw(m_queue, QColor("#FFB547"));
    draw(m_speed, QColor("#35D2C6"));
    draw(m_arrivals, QColor("#B485FF"));
}

IntersectionDetailPanel::IntersectionDetailPanel(QWidget* parent) : QWidget(parent)
{
    setObjectName("intersectionDetailRoot");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "QWidget#intersectionDetailRoot{background:rgba(5,21,36,220);color:#e4f2fc;}"
        "QWidget[metricCard=true]{background:rgba(14,43,63,178);border:1px solid rgba(71,155,199,125);border-radius:7px;}"
        "QLabel[caption=true]{color:#8ec4e2;font-size:11px;}"
        "QLabel[value=true]{color:#f2faff;font-size:16px;font-weight:600;}"
        "QTextBrowser{background:rgba(7,26,40,145);color:#cde5f4;border:1px solid rgba(71,155,199,100);padding:4px;}");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 7, 8, 8);
    root->setSpacing(6);
    m_title = new QLabel(QStringLiteral("单交叉口数据采集与可视化"), this);
    QFont title = m_title->font(); title.setBold(true); title.setPointSize(title.pointSize() + 2);
    m_title->setFont(title);
    m_meta = new QLabel(QStringLiteral("请点击路网中的青色交叉口标记"), this);
    m_meta->setWordWrap(true);
    root->addWidget(m_title);
    root->addWidget(m_meta);
    m_flow = new DataFlowCanvas(this);
    root->addWidget(m_flow);
    auto* cards = new QGridLayout();
    m_queue = makeValueCard(QStringLiteral("进口排队"), cards, 0, 0, this);
    m_vehicles = makeValueCard(QStringLiteral("检测车辆"), cards, 0, 1, this);
    m_speed = makeValueCard(QStringLiteral("平均速度"), cards, 0, 2, this);
    m_signal = makeValueCard(QStringLiteral("当前灯色"), cards, 1, 0, this);
    m_stage = makeValueCard(QStringLiteral("放行阶段"), cards, 1, 1, this);
    m_owner = makeValueCard(QStringLiteral("主控节点"), cards, 1, 2, this);
    root->addLayout(cards);
    m_trend = new IntersectionTrendPlot(this);
    root->addWidget(m_trend);
    m_detail = new QTextBrowser(this);
    m_detail->setMinimumHeight(120);
    root->addWidget(m_detail, 1);
    m_api = new QTextBrowser(this);
    m_api->setMinimumHeight(95);
    root->addWidget(m_api);
    resetView();
}

void IntersectionDetailPanel::showIntersection(int intersectionIndex,
                                               const QString& markerId,
                                               const QString& markerName,
                                               const QString& dataScopeNote)
{
    if (m_intersectionIndex != intersectionIndex) m_trend->clear();
    m_intersectionIndex = intersectionIndex;
    m_markerId = markerId;
    m_markerName = markerName;
    m_scopeNote = dataScopeNote;
    m_title->setText(QStringLiteral("%1 | %2").arg(markerId, markerName));
    m_meta->setText(dataScopeNote.isEmpty()
        ? QStringLiteral("已定位并放大；等待单路口实时采样")
        : dataScopeNote);
}

void IntersectionDetailPanel::updateTelemetry(const QVariantMap& values)
{
    if (m_intersectionIndex < 0
        || values.value("selectedIndex").toInt() != m_intersectionIndex) return;
    m_flow->setSnapshot(values);
    m_queue->setText(QStringLiteral("%1 辆").arg(values.value("intersectionQueue").toDouble(), 0, 'f', 0));
    m_vehicles->setText(QStringLiteral("%1 辆").arg(values.value("intersectionVehicles").toDouble(), 0, 'f', 0));
    m_speed->setText(QStringLiteral("%1 km/h").arg(values.value("intersectionSpeed").toDouble(), 0, 'f', 1));
    const QString color = values.value("signalColor").toString();
    const bool signalized = values.value("signalized", true).toBool();
    const bool permanentGreen = values.value("permanentGreen").toBool();
    const QString colorText = permanentGreen ? QStringLiteral("全部相位常绿")
        : color == "UNSIGNALIZED" ? QStringLiteral("无信号让行")
        : color == "GREEN" ? QStringLiteral("绿灯放行")
        : color == "YELLOW" ? QStringLiteral("黄灯过渡")
        : color == "ALL_RED" ? QStringLiteral("全红清空") : QStringLiteral("红灯禁行");
    m_signal->setText(colorText);
    m_stage->setText(permanentGreen
        ? QStringLiteral("ALL_PHASES_GREEN\n不参与相位切换")
        : signalized
        ? QStringLiteral("%1\n已运行 %2 s")
              .arg(values.value("stage").toString())
              .arg(values.value("phaseElapsed").toDouble(), 0, 'f', 1)
        : QStringLiteral("不设信控相位\nTESSNG冲突让行"));
    m_owner->setText(permanentGreen ? QStringLiteral("永久常绿")
                     : signalized ? values.value("owner").toString()
                                : QStringLiteral("本地让行"));
    m_trend->append(values.value("intersectionQueue").toDouble(),
                    values.value("intersectionSpeed").toDouble(),
                    values.value("intersectionArrivals").toDouble());

    QStringList lines;
    lines << QStringLiteral("【各阶段实时采集】");
    for (const QVariant& value : values.value("stages").toList()) {
        const QVariantMap stage = value.toMap();
        const QString stageColor = stage.value("color").toString() == "U"
            ? QStringLiteral("无信号") : stage.value("color").toString();
        lines << QStringLiteral("%1  灯色=%2  排队=%3辆  车辆=%4辆  速度=%5km/h  到达率=%6辆/min  下游排队=%7辆  占有率=%8%")
            .arg(stage.value("stageId").toString(), stageColor)
            .arg(stage.value("queueVeh").toDouble(), 0, 'f', 0)
            .arg(stage.value("vehicleCount").toDouble(), 0, 'f', 0)
            .arg(stage.value("meanSpeedKph").toDouble(), 0, 'f', 1)
            .arg(stage.value("arrivalsVehPerMin").toDouble(), 0, 'f', 1)
            .arg(stage.value("downstreamQueueVeh").toDouble(), 0, 'f', 0)
            .arg(stage.value("occupancy").toDouble() * 100.0, 0, 'f', 1);
    }
    m_detail->setPlainText(lines.join('\n'));
    const QString id = m_markerId.isEmpty() ? values.value("intersectionId").toString() : m_markerId;
    m_api->setHtml(permanentGreen ? QStringLiteral(
        "<b>状态上报</b> POST <code>/api/v1/intersections/%1/state</code><br/>"
        "输入：车辆位置/速度/轨迹、排队、占有率、全绿灯色回读<br/>"
        "<b>控制方式</b> 全部现存直行/右转相位永久绿灯，不参与固定配时或最大压力切换<br/>"
        "<b>协同输出</b> 仍保留路径代价、速度建议和路口数据可视化").arg(id)
        : signalized ? QStringLiteral(
        "<b>状态上报</b> POST <code>/api/v1/intersections/%1/state</code><br/>"
        "输入：车辆位置/速度/轨迹、排队、占有率、灯色<br/>"
        "<b>协同决策</b> POST <code>/api/v1/intersections/%1/decision</code><br/>"
        "输出：stageId、greenSec、主从租约、路径代价、速度建议<br/>"
        "<b>执行回执</b> POST <code>/api/v1/intersections/%1/ack</code>").arg(id)
        : QStringLiteral(
        "<b>状态上报</b> POST <code>/api/v1/intersections/%1/state</code><br/>"
        "输入：车辆位置/速度/轨迹、排队、占有率和冲突状态<br/>"
        "<b>本地让行</b> J253不接收灯色与相位命令，由TESSNG连接段冲突规则通行<br/>"
        "<b>协同输出</b> 仍保留路径代价、速度建议和路口数据可视化").arg(id));
    m_meta->setText(QStringLiteral("%1 · %2 · %3向 · ConnectorArea %4%5")
        .arg(values.value("junctionTypeName").toString(),
             values.value("elevationLayerName").toString())
        .arg(values.value("armCount").toInt())
        .arg(values.value("connectorAreaId").toLongLong())
        .arg(m_scopeNote.isEmpty() ? QString() : QStringLiteral(" · %1").arg(m_scopeNote)));
}

void IntersectionDetailPanel::resetView()
{
    m_flow->clear();
    m_trend->clear();
    for (QLabel* label : {m_queue, m_vehicles, m_speed, m_signal, m_stage, m_owner})
        label->setText(QStringLiteral("等待采样"));
    m_detail->setPlainText(QStringLiteral("点击路网交叉口标记后，显示该路口各阶段排队、速度、到达率、占有率和灯色变化。"));
    m_api->setHtml(QStringLiteral("状态上报 → 主从仲裁 → 控制下发 → 执行回执"));
}
