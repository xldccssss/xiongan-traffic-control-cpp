#pragma once

#include "Plugin/tessplugin.h"

class ControlPanel;
class IntersectionDetailPanel;
class IntersectionInteractionLayer;
class MyNet;
class MySimulator;
class ProjectModel;
class QDockWidget;
class TelemetryPanel;

class MyPlugin : public TessPlugin
{
public:
    MyPlugin(ProjectModel* model, const QString& projectRoot);
    ~MyPlugin() override;

    void init() override;
    void unload() override;
    CustomerGui* customerGui() override { return nullptr; }
    CustomerNet* customerNet() override;
    CustomerSimulator* customerSimulator() override;
    QString projName() override;

private:
    void positionDetailDock();

    ProjectModel* m_model = nullptr;
    QString m_projectRoot;
    MyNet* m_net = nullptr;
    MySimulator* m_simulator = nullptr;
    ControlPanel* m_controlPanel = nullptr;
    TelemetryPanel* m_telemetryPanel = nullptr;
    IntersectionDetailPanel* m_detailPanel = nullptr;
    IntersectionInteractionLayer* m_interactionLayer = nullptr;
    QDockWidget* m_controlDock = nullptr;
    QDockWidget* m_telemetryDock = nullptr;
    QDockWidget* m_detailDock = nullptr;
    bool m_initialized = false;
};
