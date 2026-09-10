#pragma once

#include <QObject>
#include <QPointer>
#include <QVector>

class ProjectModel;
class QGraphicsItem;
class IntersectionMarker;
class QGraphicsScene;
class QTimer;

class IntersectionInteractionLayer : public QObject
{
    Q_OBJECT
public:
    explicit IntersectionInteractionLayer(const ProjectModel* model,
                                          QObject* parent = nullptr);
    ~IntersectionInteractionLayer() override;

    void start();

signals:
    void intersectionActivated(int intersectionIndex,
                               const QString& markerId,
                               const QString& markerName,
                               const QString& dataScopeNote);

private:
    void install();
    void clear();
    void maintain();
    void raiseMarkers();
    void activate(QGraphicsItem* item);

    const ProjectModel* m_model = nullptr;
    QPointer<QGraphicsScene> m_scene;
    QPointer<QGraphicsScene> m_candidateScene;
    QVector<QPointer<IntersectionMarker>> m_markers;
    QPointer<IntersectionMarker> m_selected;
    QTimer* m_maintenanceTimer = nullptr;
    int m_installAttempts = 0;
    int m_lastObservedLinkCount = -1;
    int m_stableSceneTicks = 0;
};
