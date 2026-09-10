#include "MyNet.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTextStream>
#include <QTimer>

#include "ILane.h"
#include "IConnector.h"
#include "ILink.h"
#include "ISignalPhase.h"
#include "ISignalPlan.h"
#include "ITrafficLight.h"
#include "UnitChange.h"
#include "Plugin/_datastruct.h"
#include "_tables.h"
#include "core/ProjectModel.h"
#include "isignallamp.h"
#include "netinterface.h"
#include "simuinterface.h"
#include "tessinterface.h"

namespace {

QList<Online::ColorInterval> phaseColors(const IntersectionDefinition& intersection,
                                         const StageDefinition& stage)
{
    QList<Online::ColorInterval> colors;
    if (intersection.permanentGreen) {
        colors.push_back(Online::ColorInterval(
            QStringLiteral("G"), qMax(1, intersection.cycleSec)));
        return colors;
    }
    const int before = qMax(0, stage.startOffsetSec);
    const int green = qMax(1, stage.greenSec);
    const int yellow = qMax(1, stage.yellowSec);
    const int after = qMax(0, intersection.cycleSec - before - green - yellow);
    if (before > 0) colors.push_back(Online::ColorInterval(QStringLiteral("R"), before));
    colors.push_back(Online::ColorInterval(QStringLiteral("G"), green));
    colors.push_back(Online::ColorInterval(QStringLiteral("Y"), yellow));
    if (after > 0) colors.push_back(Online::ColorInterval(QStringLiteral("R"), after));
    return colors;
}

QString lampName(const IntersectionDefinition& intersection,
                 int stageIndex, qint64 laneId)
{
    return QStringLiteral("L%1_%2_%3_-1")
        .arg(intersection.id.mid(1))
        .arg(stageIndex + 1)
        .arg(laneId)
        .left(31);
}

void writeReport(const ProjectModel* model, const QJsonObject& report)
{
    if (!model) return;
    const QString path = QDir(model->root()).filePath(
        QStringLiteral("generated/native_signal_build_report.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
}

void finishSignalSmokeTestIfRequested()
{
    QCoreApplication* application = QCoreApplication::instance();
    if (application && application->arguments().contains(
            QStringLiteral("--signal-build-smoke-test"))) {
        QTimer::singleShot(0, application, &QCoreApplication::quit);
    }
}

void startRuntimeSmokeTestIfRequested(bool networkReady)
{
    QCoreApplication* application = QCoreApplication::instance();
    if (!application || !application->arguments().contains(
            QStringLiteral("--runtime-smoke-test"))) return;
    if (!networkReady) {
        QTimer::singleShot(0, application, &QCoreApplication::quit);
        return;
    }
    QTimer::singleShot(0, application, []() {
        if (gpTessInterface && gpTessInterface->simuInterface())
            gpTessInterface->simuInterface()->startSimu();
    });
}

bool ensureConfiguredVehicleTypes(NetInterface* net, const ProjectModel* model,
                                  QJsonArray* errors, QJsonArray* warnings,
                                  int* createdCount)
{
    if (!net || !model) return false;
    QHash<long, _VehicleType> existing;
    for (const _VehicleType& type : net->vehicleTypes())
        existing.insert(type.vehicleTypeCode, type);
    const QJsonArray specs = model->scenario().value("initialTraffic")
        .toObject().value("vehicleTypes").toArray();
    QList<Online::VehiComposition> composition;
    *createdCount = 0;
    for (const QJsonValue& raw : specs) {
        const QJsonObject spec = raw.toObject();
        const long code = spec.value("vehicleTypeCode").toInt();
        const QString name = spec.value("vehicleTypeName").toString();
        if (code <= 0 || name.isEmpty()) {
            errors->push_back(QStringLiteral("invalid configured vehicle type"));
            continue;
        }
        if (existing.contains(code)) {
            // TESSNG's built-in type 1 is named "小客车". The configured
            // "CAV小客车" text is a display alias; behaviour is keyed by the
            // stable type code plus CAV metadata, not by the localized label.
            if (existing.value(code).vehicleTypeName != name)
                warnings->push_back(QStringLiteral(
                    "vehicle type %1 uses SDK name '%2'; scenario alias is '%3'")
                    .arg(code).arg(existing.value(code).vehicleTypeName, name));
        } else if (spec.value("createIfMissing").toBool(false)) {
            const QJsonObject definition = spec.value("definition").toObject();
            const QStringList required{
                "Length", "lengthSD", "width", "widthSD", "avgSpeed",
                "speedSD", "maxSpeed", "avgAcce", "acceSD", "maxAcce",
                "avgDece", "deceSD", "maxDece", "commercialVehicle",
                "maxPassengerCapacity", "initialPassengerCount",
                "vehicleTypeCodeBase"
            };
            QStringList missing;
            for (const QString& field : required)
                if (!definition.contains(field)) missing.push_back(field);
            if (!missing.isEmpty()) {
                errors->push_back(QStringLiteral("车型%1缺少参数：%2")
                    .arg(name, missing.join(",")));
                continue;
            }
            _VehicleType type;
            type.vehicleTypeCode = code;
            type.vehicleTypeName = name;
            type.Length = definition.value("Length").toDouble();
            type.lengthSD = definition.value("lengthSD").toDouble();
            type.width = definition.value("width").toDouble();
            type.widthSD = definition.value("widthSD").toDouble();
            type.avgSpeed = definition.value("avgSpeed").toDouble();
            type.speedSD = definition.value("speedSD").toDouble();
            type.maxSpeed = definition.value("maxSpeed").toDouble();
            type.avgAcce = definition.value("avgAcce").toDouble();
            type.acceSD = definition.value("acceSD").toDouble();
            type.maxAcce = definition.value("maxAcce").toDouble();
            type.avgDece = definition.value("avgDece").toDouble();
            type.deceSD = definition.value("deceSD").toDouble();
            type.maxDece = definition.value("maxDece").toDouble();
            const QJsonValue commercial = definition.value("commercialVehicle");
            type.commercialVehicle = commercial.isBool()
                ? commercial.toBool() : commercial.toInt() != 0;
            type.maxPassengerCapacity = definition.value("maxPassengerCapacity").toInt();
            type.initialPassengerCount = definition.value("initialPassengerCount").toInt();
            type.vehicleTypeCodeBase = definition.value("vehicleTypeCodeBase").toInt();
            if (!net->createVehicleType(type)) {
                errors->push_back(QStringLiteral("createVehicleType创建%1(编码%2)失败")
                    .arg(name).arg(code));
                continue;
            }
            existing.insert(code, type);
            ++*createdCount;
        } else {
            errors->push_back(QStringLiteral("缺少必需车型%1(编码%2)").arg(name).arg(code));
            continue;
        }
        composition.push_back(Online::VehiComposition(
            code, qMax(0.0, spec.value("share").toDouble())));
    }
    if (composition.size() != specs.size() || composition.isEmpty()) return false;
    const long compositionId = model->scenario().value("demand").toObject()
        .value("vehicleCompositionId").toInt(1);
    bool compositionExists = false;
    for (const Online::VehicleCompositionList& item : net->vehicleCompositions())
        compositionExists = compositionExists || item.vehicleConsCode == compositionId;
    if (!compositionExists) {
        errors->push_back(QStringLiteral("目标车辆组成ID %1不存在").arg(compositionId));
        return false;
    }
    if (!net->updateVehicleComposition(compositionId, composition)) {
        errors->push_back(QStringLiteral("车辆组成ID %1更新失败").arg(compositionId));
        return false;
    }
    return errors->isEmpty();
}

} // namespace

MyNet::MyNet(ProjectModel* model)
    : m_model(model)
{
}

void MyNet::afterLoadNet()
{
    QJsonObject report;
    report.insert(QStringLiteral("timestamp"),
                  QDateTime::currentDateTime().toString(Qt::ISODate));
    report.insert(QStringLiteral("policy"),
                  QStringLiteral("DELETE_ALL_AND_REBUILD_PHYSICAL_LANE_ATOMIC"));

    NetInterface* net = gpTessInterface ? gpTessInterface->netInterface() : nullptr;
    if (!net || !m_model) {
        report.insert(QStringLiteral("success"), false);
        report.insert(QStringLiteral("error"), QStringLiteral("netInterface or ProjectModel is null"));
        writeReport(m_model, report);
        finishSignalSmokeTestIfRequested();
        return;
    }

    // Preflight every configured physical incoming lane before deleting any
    // existing signal object.  A bad manifest must never leave a partly built
    // network or silently omit one intersection.
    QJsonArray errors;
    QJsonArray warnings;
    QJsonArray removedDisabledConnectors;
    QJsonArray alreadyAbsentDisabledConnectors;
    for (qint64 connectorId : m_model->disabledConnectorIds()) {
        IConnector* connector = net->findConnector(static_cast<long>(connectorId));
        if (!connector) {
            alreadyAbsentDisabledConnectors.append(
                static_cast<double>(connectorId));
            continue;
        }
        const qint64 fromLinkId = connector->fromLink()
            ? connector->fromLink()->id() : 0;
        const qint64 toLinkId = connector->toLink()
            ? connector->toLink()->id() : 0;
        if (!m_model->isMovementDisabled(fromLinkId, toLinkId)) {
            errors.push_back(QStringLiteral(
                "disabled connector %1 endpoint mismatch: %2 -> %3")
                .arg(connectorId).arg(fromLinkId).arg(toLinkId));
            continue;
        }
        net->removeConnector(connector);
        if (net->findConnector(static_cast<long>(connectorId))) {
            errors.push_back(QStringLiteral(
                "failed to remove disabled connector %1 (%2 -> %3)")
                .arg(connectorId).arg(fromLinkId).arg(toLinkId));
        } else {
            removedDisabledConnectors.append(static_cast<double>(connectorId));
        }
    }
    report.insert(QStringLiteral("removedDisabledConnectors"),
                  removedDisabledConnectors);
    report.insert(QStringLiteral("alreadyAbsentDisabledConnectors"),
                  alreadyAbsentDisabledConnectors);
    int vehicleTypesCreated = 0;
    const bool vehicleTypesReady = ensureConfiguredVehicleTypes(
        net, m_model, &errors, &warnings, &vehicleTypesCreated);
    report.insert(QStringLiteral("vehicleTypesCreated"), vehicleTypesCreated);
    report.insert(QStringLiteral("vehicleCompositionUpdated"), vehicleTypesReady);
    report.insert(QStringLiteral("warnings"), warnings);
    QSet<qint64> configuredLanes;
    QJsonArray unsignalizedIntersectionIds;
    QJsonArray permanentGreenIntersectionIds;
    int expectedControllers = 0;
    int expectedPhases = 0;
    for (const IntersectionDefinition& intersection : m_model->intersections()) {
        if (!intersection.signalized) {
            unsignalizedIntersectionIds.append(intersection.id);
            continue;
        }
        if (intersection.permanentGreen)
            permanentGreenIntersectionIds.append(intersection.id);
        ++expectedControllers;
        QSet<qint64> lanesAtIntersection;
        expectedPhases += intersection.stages.size();
        for (const StageDefinition& stage : intersection.stages) {
            for (qint64 laneId : stage.incomingLaneIds) {
                if (!net->findLane(static_cast<long>(laneId))) {
                    errors.push_back(QStringLiteral("%1/%2: missing lane %3")
                                         .arg(intersection.id, stage.id)
                                         .arg(laneId));
                }
                if (lanesAtIntersection.contains(laneId)) {
                    errors.push_back(QStringLiteral("%1: lane %2 belongs to more than one stage")
                                         .arg(intersection.id).arg(laneId));
                }
                if (configuredLanes.contains(laneId)) {
                    errors.push_back(QStringLiteral("lane %1 belongs to more than one intersection")
                                         .arg(laneId));
                }
                lanesAtIntersection.insert(laneId);
                configuredLanes.insert(laneId);
            }
        }
    }
    report.insert(QStringLiteral("expectedControllers"), expectedControllers);
    report.insert(QStringLiteral("expectedPhases"), expectedPhases);
    report.insert(QStringLiteral("expectedLamps"), configuredLanes.size());
    report.insert(QStringLiteral("unsignalizedIntersectionIds"),
                  unsignalizedIntersectionIds);
    report.insert(QStringLiteral("permanentGreenIntersectionIds"),
                  permanentGreenIntersectionIds);
    QJsonArray mergedIntersections;
    for (const IntersectionDefinition& intersection : m_model->intersections()) {
        if (intersection.mergedMemberIds.isEmpty()) continue;
        QJsonObject merged;
        merged.insert(QStringLiteral("primaryIntersectionId"), intersection.id);
        QJsonArray members;
        for (const QString& memberId : intersection.mergedMemberIds)
            members.append(memberId);
        merged.insert(QStringLiteral("memberIntersectionIds"), members);
        mergedIntersections.append(merged);
    }
    report.insert(QStringLiteral("mergedIntersections"), mergedIntersections);
    if (!vehicleTypesReady || !errors.isEmpty()) {
        report.insert(QStringLiteral("success"), false);
        report.insert(QStringLiteral("errors"), errors);
        writeReport(m_model, report);
        qCritical("Native signal rebuild preflight failed with %d errors", errors.size());
        finishSignalSmokeTestIfRequested();
        startRuntimeSmokeTestIfRequested(false);
        return;
    }

    int removedLamps = 0;
    const QList<ISignalLamp*> oldLamps = net->signalLamps();
    for (ISignalLamp* lamp : oldLamps)
        if (lamp && net->removeSignalLamp(lamp)) ++removedLamps;

    int removedPhases = 0;
    int removedPlans = 0;
    int removedControllers = 0;
    const QList<ISignalController*> oldControllers = net->signalControllers();
    for (ISignalController* controller : oldControllers) {
        if (!controller) continue;
        const QList<ISignalPlan*> plans = controller->plans();
        for (ISignalPlan* plan : plans) {
            if (!plan) continue;
            const QList<ISignalPhase*> phases = plan->phases();
            for (ISignalPhase* phase : phases) {
                if (phase && net->removePhase(phase)) ++removedPhases;
            }
            if (net->removeSignalPlan(plan)) ++removedPlans;
        }
        if (net->removeSignalController(controller)) ++removedControllers;
    }

    int createdControllers = 0;
    int createdPlans = 0;
    int createdPhases = 0;
    int createdLamps = 0;
    for (const IntersectionDefinition& intersection : m_model->intersections()) {
        if (!intersection.signalized) continue;
        ISignalController* controller = net->createSignalController(
            QStringLiteral("AUTO_SIG_%1").arg(intersection.id));
        if (!controller) {
            errors.push_back(QStringLiteral("%1: createSignalController failed")
                                 .arg(intersection.id));
            continue;
        }
        ++createdControllers;
        ISignalPlan* plan = net->createSignalPlan(
            controller, QStringLiteral("AUTO_PLAN_%1").arg(intersection.id),
            qMax(1, intersection.cycleSec), 0, 0, 86400);
        if (!plan) {
            errors.push_back(QStringLiteral("%1: createSignalPlan failed")
                                 .arg(intersection.id));
            continue;
        }
        ++createdPlans;

        for (int stageIndex = 0; stageIndex < intersection.stages.size(); ++stageIndex) {
            const StageDefinition& stage = intersection.stages[stageIndex];
            ISignalPhase* phase = net->createSignalPlanSignalPhase(
                plan, QStringLiteral("AUTO_%1").arg(stage.id),
                phaseColors(intersection, stage));
            if (!phase) {
                errors.push_back(QStringLiteral("%1/%2: create phase failed")
                                     .arg(intersection.id, stage.id));
                continue;
            }
            ++createdPhases;
            for (qint64 laneId : stage.incomingLaneIds) {
                ILane* lane = net->findLane(static_cast<long>(laneId));
                const qreal distance = m2p(qMax<qreal>(0.0, lane->length(Metric) - 0.5));
                ISignalLamp* lamp = net->createTrafficSignalLamp(
                    controller, lampName(intersection, stageIndex, laneId),
                    static_cast<long>(laneId), -1, distance);
                if (!lamp) {
                    errors.push_back(QStringLiteral("%1/%2: create lamp for lane %3 failed")
                                         .arg(intersection.id, stage.id).arg(laneId));
                    continue;
                }
                net->addSignalPhaseToLamp(static_cast<int>(phase->id()), lamp);
                lamp->setLampColor(intersection.permanentGreen
                    ? QStringLiteral("G") : QStringLiteral("R"));
                ++createdLamps;
            }
        }
    }

    report.insert(QStringLiteral("removedLamps"), removedLamps);
    report.insert(QStringLiteral("removedPhases"), removedPhases);
    report.insert(QStringLiteral("removedPlans"), removedPlans);
    report.insert(QStringLiteral("removedControllers"), removedControllers);
    report.insert(QStringLiteral("createdControllers"), createdControllers);
    report.insert(QStringLiteral("createdPlans"), createdPlans);
    report.insert(QStringLiteral("createdPhases"), createdPhases);
    report.insert(QStringLiteral("createdLamps"), createdLamps);
    const int observedControllers = net->signalControllers().size();
    const int observedLamps = net->signalLamps().size();
    report.insert(QStringLiteral("observedControllers"), observedControllers);
    report.insert(QStringLiteral("observedLamps"), observedLamps);
    report.insert(QStringLiteral("errors"), errors);
    const bool success = errors.isEmpty()
        && createdControllers == expectedControllers
        && createdPhases == expectedPhases
        && createdLamps == configuredLanes.size()
        && observedControllers == createdControllers
        && observedLamps == createdLamps;
    report.insert(QStringLiteral("success"), success);
    writeReport(m_model, report);
    qInfo("Native signal rebuild: controllers=%d phases=%d lamps=%d success=%d",
          createdControllers, createdPhases, createdLamps, success ? 1 : 0);
    finishSignalSmokeTestIfRequested();
    startRuntimeSmokeTestIfRequested(success);
}
