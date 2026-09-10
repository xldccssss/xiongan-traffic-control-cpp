#include "MyPlugin.h"

#include <QDockWidget>
#include <QJsonObject>
#include <QMainWindow>
#include <QMetaObject>
#include <QPoint>
#include <QTimer>

#include "MyNet.h"
#include "MySimulator.h"
#include "core/ProjectModel.h"
#include "guiinterface.h"
#include "tessinterface.h"
#include "ui/ControlPanel.h"
#include "ui/IntersectionDetailPanel.h"
#include "ui/IntersectionInteractionLayer.h"
#include "ui/TelemetryPanel.h"

MyPlugin::MyPlugin(ProjectModel* model, const QString& projectRoot)
    : m_model(model), m_projectRoot(projectRoot)
{
}

MyPlugin::~MyPlugin()
{
    unload();
    delete m_model;
    m_model = nullptr;
}

void MyPlugin::init()
{
    if (m_initialized) return;
    m_initialized = true;
    m_net = new MyNet(m_model);
    m_simulator = new MySimulator(m_model);

    const QJsonObject randomSeeds = m_model->scenario().value("randomSeeds").toObject();
    const int preloadMaximum = m_model->placements().size();
    const int preloadCount = qBound(
        0,
        m_model->scenario().value("initialTraffic").toObject()
            .value("targetVehicleCount").toInt(preloadMaximum),
        preloadMaximum);
    const QJsonObject demand = m_model->scenario().value("demand").toObject();
    int outerEntryCount = 0;
    int residentialEntryCount = 0;
    for (const TerminalDefinition& terminal : m_model->terminals()) {
        if (terminal.group == QStringLiteral("RESIDENTIAL_AREA"))
            ++residentialEntryCount;
        else
            ++outerEntryCount;
    }
    const QString initialAlgorithm = m_model->scenario().value("runtimeControl")
        .toObject().value("activeBranch").toString(
            QStringLiteral("IMPROVED_MAX_PRESSURE"));
    m_controlPanel = new ControlPanel(
        randomSeeds.value("vehicleGeneration").toInt(20260822),
        randomSeeds.value("destinationChoice").toInt(20260823),
        preloadCount,
        preloadMaximum,
        demand.value("durationSec").toInt(3600),
        demand.value("outerBoundaryVehiclesPerEntryPerHour").toInt(1000),
        demand.value("residentialVehiclesPerEntryPerHour").toInt(100),
        outerEntryCount,
        residentialEntryCount,
        m_model->scenario().value("simulationPanel").toObject()
            .value("comparisonRounds").toInt(3),
        initialAlgorithm);
    m_telemetryPanel = new TelemetryPanel();
    m_detailPanel = new IntersectionDetailPanel();
    QMainWindow* window = gpTessInterface->guiInterface()->mainWindow();

    m_controlDock = new QDockWidget(QStringLiteral("雄安窄密路网仿真控制"), window);
    m_controlDock->setObjectName("XionganNativeCppControlDock");
    m_controlDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_controlDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_controlDock->setWidget(m_controlPanel);
    gpTessInterface->guiInterface()->addDockWidgetToMainWindow(Qt::RightDockWidgetArea,
                                                               m_controlDock);

    m_telemetryDock = new QDockWidget(QStringLiteral("数据采集与可视化"), window);
    m_telemetryDock->setObjectName("XionganNativeCppTelemetryDock");
    m_telemetryDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_telemetryDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea);
    m_telemetryDock->setWidget(m_telemetryPanel);
    m_telemetryDock->setMinimumHeight(265);
    gpTessInterface->guiInterface()->addDockWidgetToMainWindow(Qt::BottomDockWidgetArea,
                                                               m_telemetryDock);
    QTimer::singleShot(0, window, [window, this]() {
        if (m_controlDock)
            window->resizeDocks({m_controlDock}, {295}, Qt::Horizontal);
        if (m_telemetryDock)
            window->resizeDocks({m_telemetryDock}, {300}, Qt::Vertical);
    });

    m_detailDock = new QDockWidget(QStringLiteral("单交叉口数据采集与可视化"), window);
    m_detailDock->setObjectName("XionganNativeCppIntersectionDetailDock");
    m_detailDock->setFeatures(QDockWidget::DockWidgetMovable
                              | QDockWidget::DockWidgetFloatable
                              | QDockWidget::DockWidgetClosable);
    m_detailDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                  | Qt::BottomDockWidgetArea);
    m_detailDock->setWidget(m_detailPanel);
    gpTessInterface->guiInterface()->addDockWidgetToMainWindow(Qt::LeftDockWidgetArea,
                                                               m_detailDock);
    const QJsonObject interaction = m_model->scenario().value("dataVisualizationPanel")
        .toObject().value("intersectionInteraction").toObject();
    m_detailDock->setFloating(true);
    // Showing the floating detail window must not steal the mouse/focus from
    // TESSNG's road-network view, otherwise subsequent pan gestures stop working.
    m_detailDock->setAttribute(Qt::WA_ShowWithoutActivating, true);
    const int detailWidth = qMin(qMax(600, interaction.value("detailWindowWidth").toInt(720)),
                                 qMax(600, window->width() / 2));
    const int detailHeight = qMin(qMax(640, interaction.value("detailWindowHeight").toInt(780)),
                                  qMax(640, window->height() - 100));
    m_detailDock->resize(detailWidth, detailHeight);
    m_detailDock->hide();

    QObject::connect(m_controlPanel, &ControlPanel::startRequested,
                     m_simulator, &MySimulator::requestStart, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::pauseRequested,
                     m_simulator, &MySimulator::requestPause, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::resetRequested,
                     m_simulator, &MySimulator::requestReset, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::stopRequested,
                     m_simulator, &MySimulator::requestStop, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::algorithmChanged,
                     m_simulator, &MySimulator::setAlgorithm, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::speedChanged,
                     m_simulator, &MySimulator::setTargetMultiple, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::outerRateChanged,
                     m_simulator, &MySimulator::setOuterRate, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::residentialRateChanged,
                     m_simulator, &MySimulator::setResidentialRate, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::durationChanged,
                     m_simulator, &MySimulator::setDurationSec, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::randomSeedsChanged,
                     m_simulator, &MySimulator::setRandomSeeds, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::preloadCountChanged,
                     m_simulator, &MySimulator::setPreloadCount, Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::comparisonRequested,
                     m_simulator, &MySimulator::requestComparison,
                     Qt::QueuedConnection);
    QObject::connect(m_controlPanel, &ControlPanel::comparisonCancelRequested,
                     m_simulator, &MySimulator::requestCancelComparison,
                     Qt::QueuedConnection);
    QObject::connect(m_simulator, &MySimulator::comparisonStateChanged,
                     m_controlPanel, &ControlPanel::updateComparisonState,
                     Qt::QueuedConnection);
    QObject::connect(m_simulator, &MySimulator::telemetryUpdated,
                     m_telemetryPanel, &TelemetryPanel::updateTelemetry, Qt::QueuedConnection);
    QObject::connect(m_simulator, &MySimulator::telemetryUpdated,
                     m_detailPanel, &IntersectionDetailPanel::updateTelemetry,
                     Qt::QueuedConnection);
    QObject::connect(m_simulator, &MySimulator::eventGenerated,
                     m_telemetryPanel, &TelemetryPanel::appendEvent, Qt::QueuedConnection);
    QObject::connect(m_simulator, &MySimulator::viewResetRequested,
                     m_telemetryPanel, &TelemetryPanel::resetView, Qt::QueuedConnection);
    QObject::connect(m_simulator, &MySimulator::viewResetRequested,
                     m_detailPanel, &IntersectionDetailPanel::resetView,
                     Qt::QueuedConnection);

    m_interactionLayer = new IntersectionInteractionLayer(m_model, window);
    QObject::connect(m_interactionLayer,
                     &IntersectionInteractionLayer::intersectionActivated,
                     window,
                     [this](int index, const QString& markerId,
                            const QString& markerName, const QString& dataScopeNote) {
        m_detailPanel->showIntersection(index, markerId, markerName, dataScopeNote);
        QMetaObject::invokeMethod(m_simulator, "setSelectedIntersection",
                                  Qt::QueuedConnection, Q_ARG(int, index));
        m_detailDock->show();
        positionDetailDock();
        QTimer::singleShot(0, m_detailDock, [this]() { positionDetailDock(); });
        m_detailDock->raise();
    });
    m_interactionLayer->start();
}

void MyPlugin::positionDetailDock()
{
    if (!m_detailDock || !m_model) return;
    QMainWindow* window = gpTessInterface->guiInterface()->mainWindow();
    if (!window) return;
    const QJsonObject interaction = m_model->scenario().value("dataVisualizationPanel")
        .toObject().value("intersectionInteraction").toObject();
    const QPoint origin = window->mapToGlobal(QPoint(0, 0));
    m_detailDock->move(origin.x()
                           + qMax(0, interaction.value("detailWindowLeftMarginPx").toInt(16)),
                       origin.y()
                           + qMax(0, interaction.value("detailWindowTopOffsetPx").toInt(72)));
}

void MyPlugin::unload()
{
    if (!m_initialized) return;
    m_initialized = false;
    delete m_interactionLayer;
    m_interactionLayer = nullptr;
    delete m_simulator;
    m_simulator = nullptr;
    delete m_net;
    m_net = nullptr;
    if (m_controlDock) m_controlDock->deleteLater();
    if (m_telemetryDock) m_telemetryDock->deleteLater();
    if (m_detailDock) m_detailDock->deleteLater();
    m_controlDock = nullptr;
    m_telemetryDock = nullptr;
    m_detailDock = nullptr;
    m_controlPanel = nullptr;
    m_telemetryPanel = nullptr;
    m_detailPanel = nullptr;
}

CustomerNet* MyPlugin::customerNet()
{
    return m_net;
}

CustomerSimulator* MyPlugin::customerSimulator()
{
    return m_simulator;
}

QString MyPlugin::projName()
{
    return QStringLiteral("雄安窄密路网 C++ 版");
}
