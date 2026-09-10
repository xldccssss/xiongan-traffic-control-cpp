#include "IntersectionInteractionLayer.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTimer>
#include <functional>

#include "UnitChange.h"
#include "core/ProjectModel.h"
#include "guiinterface.h"
#include "netinterface.h"
#include "tessinterface.h"

class IntersectionMarker final : public QGraphicsObject
{
public:
    IntersectionMarker(int intersectionIndex, const QString& markerId,
                       const QString& markerName, const QString& note,
                       const QPointF& centerM, qreal radius,
                       std::function<void(QGraphicsItem*)> callback)
        : intersectionIndex(intersectionIndex)
        , markerId(markerId)
        , markerName(markerName)
        , dataScopeNote(note)
        , centerM(centerM)
        , m_radius(radius)
        , callback(std::move(callback))
    {
        setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptHoverEvents(true);
        setZValue(1.0e12);
        setToolTip(QStringLiteral("点击定位并放大 %1\n查看单交叉口数据与车—路—云数据流")
                       .arg(markerName));
        label = new QGraphicsSimpleTextItem(markerId, this);
        QFont font;
        font.setPixelSize(10);
        font.setBold(true);
        label->setFont(font);
        label->setPos(radius + 3.0, -label->boundingRect().height() / 2.0);
        label->setZValue(1.0);
        label->setVisible(false);
        applyStyle(false);
    }

    QRectF boundingRect() const override
    {
        return QRectF(-m_radius - 2.0, -m_radius - 2.0,
                      (m_radius + 2.0) * 2.0, (m_radius + 2.0) * 2.0);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        painter->drawEllipse(QRectF(-m_radius, -m_radius,
                                    m_radius * 2.0, m_radius * 2.0));
    }

    void applyStyle(bool selected)
    {
        isSelectedMarker = selected;
        if (selected) {
            m_pen = QPen(QColor(255, 218, 72, 255), 2.2);
            m_brush = QBrush(QColor(255, 151, 45, 245));
            label->setBrush(QBrush(QColor(255, 235, 127)));
            label->setVisible(true);
        } else {
            m_pen = QPen(QColor(106, 255, 242, 255), 1.8);
            m_brush = QBrush(QColor(12, 178, 205, 235));
            label->setBrush(QBrush(QColor(232, 249, 255)));
            label->setVisible(false);
        }
        update();
    }

    int intersectionIndex = -1;
    QString markerId;
    QString markerName;
    QString dataScopeNote;
    QPointF centerM;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override
    {
        label->setVisible(true);
        m_brush = QBrush(QColor(43, 225, 215, 255));
        update();
        event->accept();
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
    {
        applyStyle(isSelectedMarker);
        event->accept();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_clickArmed = true;
            m_pressScreenPos = event->screenPos();
            event->accept();
        } else {
            event->ignore();
        }
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        if ((event->screenPos() - m_pressScreenPos).manhattanLength()
            > QApplication::startDragDistance()) {
            m_clickArmed = false;
        }
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        const bool activateOnRelease = event->button() == Qt::LeftButton
            && m_clickArmed
            && (event->screenPos() - m_pressScreenPos).manhattanLength()
                <= QApplication::startDragDistance();
        m_clickArmed = false;
        if (!activateOnRelease) {
            event->ignore();
            return;
        }
        event->accept();
        // Run after the complete press/release sequence so TESSNG's graphics
        // view cannot be left with a stale mouse grab when the detail window opens.
        QTimer::singleShot(0, this, [this]() { callback(this); });
    }

private:
    QGraphicsSimpleTextItem* label = nullptr;
    qreal m_radius = 7.0;
    QPen m_pen;
    QBrush m_brush;
    std::function<void(QGraphicsItem*)> callback;
    bool isSelectedMarker = false;
    bool m_clickArmed = false;
    QPoint m_pressScreenPos;
};

IntersectionInteractionLayer::IntersectionInteractionLayer(
    const ProjectModel* model, QObject* parent)
    : QObject(parent), m_model(model)
{
}

IntersectionInteractionLayer::~IntersectionInteractionLayer()
{
    clear();
}

void IntersectionInteractionLayer::start()
{
    m_installAttempts = 0;
    m_lastObservedLinkCount = -1;
    m_stableSceneTicks = 0;
    if (!m_maintenanceTimer) {
        m_maintenanceTimer = new QTimer(this);
        m_maintenanceTimer->setInterval(750);
        connect(m_maintenanceTimer, &QTimer::timeout,
                this, &IntersectionInteractionLayer::maintain);
    }
    m_maintenanceTimer->start();
    QTimer::singleShot(750, this, &IntersectionInteractionLayer::maintain);
}

void IntersectionInteractionLayer::clear()
{
    if (m_scene) {
        for (const QPointer<IntersectionMarker>& marker : m_markers) {
            if (!marker) continue;
            if (marker->scene() == m_scene) m_scene->removeItem(marker);
            delete marker;
        }
    }
    m_markers.clear();
    m_selected = nullptr;
    m_scene.clear();
}

void IntersectionInteractionLayer::maintain()
{
    NetInterface* net = gpTessInterface ? gpTessInterface->netInterface() : nullptr;
    QGraphicsScene* scene = net ? net->graphicsScene() : nullptr;
    const int linkCount = net ? net->linkCount() : 0;
    if (!m_model || !net || !scene || linkCount <= 0) return;

    bool markerMissing = m_markers.isEmpty();
    for (const QPointer<IntersectionMarker>& marker : m_markers)
        markerMissing = markerMissing || marker.isNull();
    const bool sceneReplaced = m_scene && m_scene != scene;
    if (sceneReplaced || markerMissing) {
        if (sceneReplaced || !m_markers.isEmpty()) clear();
        if (m_candidateScene == scene && m_lastObservedLinkCount == linkCount)
            ++m_stableSceneTicks;
        else {
            m_candidateScene = scene;
            m_lastObservedLinkCount = linkCount;
            m_stableSceneTicks = 1;
        }
        // Avoid the early-build window in which TESSNG clears and repopulates
        // its scene. Three stable observations mean at least 1.5 seconds with
        // an unchanged loaded network before markers are attached.
        if (m_stableSceneTicks >= 3) install();
        return;
    }
    raiseMarkers();
}

void IntersectionInteractionLayer::raiseMarkers()
{
    if (!m_scene) return;
    constexpr qreal markerZ = 1.0e12;
    for (const QPointer<IntersectionMarker>& marker : m_markers) {
        if (!marker) continue;
        if (marker->scene() != m_scene) m_scene->addItem(marker);
        marker->setZValue(markerZ);
        marker->setEnabled(true);
        marker->setVisible(true);
    }
}

void IntersectionInteractionLayer::install()
{
    ++m_installAttempts;
    NetInterface* net = gpTessInterface ? gpTessInterface->netInterface() : nullptr;
    QGraphicsScene* scene = net ? net->graphicsScene() : nullptr;
    if (!m_model || !net || !scene || net->linkCount() <= 0) {
        return;
    }

    clear();
    m_scene = scene;
    const QJsonObject interaction = m_model->scenario()
        .value("dataVisualizationPanel").toObject()
        .value("intersectionInteraction").toObject();
    if (!interaction.value("enabled").toBool(true)) return;
    const qreal radius = qMax(4.0, interaction.value("markerRadiusPx").toDouble(7.0));
    const QJsonObject corrections = interaction.value("markerCorrections").toObject();

    for (int index = 0; index < m_model->intersections().size(); ++index) {
        const IntersectionDefinition& intersection = m_model->intersections()[index];
        QJsonArray specs = corrections.value(intersection.id).toArray();
        if (specs.isEmpty()) {
            QJsonObject normal;
            normal.insert("markerId", intersection.id);
            normal.insert("markerName", intersection.name);
            QJsonArray center;
            center.push_back(intersection.centerM.x());
            center.push_back(intersection.centerM.y());
            normal.insert("centerXY", center);
            specs.push_back(normal);
        }
        for (const QJsonValue& value : specs) {
            const QJsonObject spec = value.toObject();
            const QJsonArray center = spec.value("centerXY").toArray();
            if (center.size() < 2) continue;
            const QPointF centerM(center.at(0).toDouble(), center.at(1).toDouble());
            auto* marker = new IntersectionMarker(
                index,
                spec.value("markerId").toString(intersection.id),
                spec.value("markerName").toString(intersection.name),
                spec.value("dataScopeNote").toString(), centerM, radius,
                [this](QGraphicsItem* item) { activate(item); });
            marker->setPos(m2p(centerM.x()), m2p(centerM.y()));
            scene->addItem(marker);
            m_markers.push_back(marker);
        }
    }
    raiseMarkers();

    QJsonObject report;
    report.insert("dataIntersections", m_model->intersections().size());
    report.insert("clickMarkers", m_markers.size());
    report.insert("sceneInstalled", !m_markers.isEmpty());
    report.insert("markerZValue", 1.0e12);
    report.insert("topLayerMaintenanceEnabled", true);
    report.insert("correctionPolicy", QStringLiteral("J002/J004/J009/J039/J244_PHYSICAL_MARKERS_AND_J253_J254_MERGE"));
    const QString reportPath = QDir(m_model->root()).filePath(
        QStringLiteral("generated/native_interaction_install_report.json"));
    QFile reportFile(reportPath);
    if (reportFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        reportFile.write(QJsonDocument(report).toJson(QJsonDocument::Indented));

    QCoreApplication* application = QCoreApplication::instance();
    if (application && application->arguments().contains(
            QStringLiteral("--ui-interaction-smoke-test")) && !m_markers.isEmpty()) {
        activate(m_markers.first());
        QTimer::singleShot(200, this, [this, reportPath, report]() mutable {
            QDockWidget* detail = gpTessInterface->guiInterface()->mainWindow()
                ->findChild<QDockWidget*>("XionganNativeCppIntersectionDetailDock");
            report.insert("detailDockVisible", detail && detail->isVisible());
            report.insert("activationDelivered", m_selected != nullptr);
            QFile finalFile(reportPath);
            if (finalFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                finalFile.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
            QCoreApplication::quit();
        });
    }
}

void IntersectionInteractionLayer::activate(QGraphicsItem* item)
{
    auto* marker = dynamic_cast<IntersectionMarker*>(item);
    if (!marker || !m_model) return;
    if (IntersectionMarker* previous = m_selected.data())
        previous->applyStyle(false);
    marker->applyStyle(true);
    m_selected = marker;

    const QJsonObject interaction = m_model->scenario()
        .value("dataVisualizationPanel").toObject()
        .value("intersectionInteraction").toObject();
    if (interaction.value("zoomOnClick").toBool(true)) {
        NetInterface* net = gpTessInterface->netInterface();
        QGraphicsView* view = net ? net->graphicsView() : nullptr;
        if (view) {
            const qreal centerX = m2p(marker->centerM.x());
            const qreal centerY = m2p(marker->centerM.y());
            const qreal radiusM = qMax(30.0, interaction.value("focusRadiusM").toDouble(110.0));
            qreal radiusPx = qAbs(m2p(radiusM) - m2p(0.0));
            if (radiusPx < 1e-6) radiusPx = radiusM;
            view->fitInView(QRectF(centerX - radiusPx, centerY - radiusPx,
                                   radiusPx * 2.0, radiusPx * 2.0),
                            Qt::KeepAspectRatio);
            view->centerOn(QPointF(centerX, centerY));
            view->viewport()->update();
        }
    }
    emit intersectionActivated(marker->intersectionIndex, marker->markerId,
                               marker->markerName, marker->dataScopeNote);
}
