#pragma once

#include <QVariantMap>
#include <QVector>
#include <QWidget>

class QLabel;
class QTextBrowser;
class QTimer;

class DataFlowCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit DataFlowCanvas(QWidget* parent = nullptr);
    void setSnapshot(const QVariantMap& values);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVariantMap m_values;
    QTimer* m_animation = nullptr;
    double m_progress = 0.0;
    double m_previousQueue = 0.0;
    double m_previousSpeed = 0.0;
    double m_queueDelta = 0.0;
    double m_speedDelta = 0.0;
    bool m_hasPrevious = false;
};

class IntersectionTrendPlot : public QWidget
{
    Q_OBJECT
public:
    explicit IntersectionTrendPlot(QWidget* parent = nullptr);
    void append(double queue, double speed, double arrivals);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_queue;
    QVector<double> m_speed;
    QVector<double> m_arrivals;
};

class IntersectionDetailPanel : public QWidget
{
    Q_OBJECT
public:
    explicit IntersectionDetailPanel(QWidget* parent = nullptr);

public slots:
    void showIntersection(int intersectionIndex, const QString& markerId,
                          const QString& markerName, const QString& dataScopeNote);
    void updateTelemetry(const QVariantMap& values);
    void resetView();

private:
    int m_intersectionIndex = -1;
    QString m_markerId;
    QString m_markerName;
    QString m_scopeNote;
    QLabel* m_title = nullptr;
    QLabel* m_meta = nullptr;
    QLabel* m_queue = nullptr;
    QLabel* m_vehicles = nullptr;
    QLabel* m_speed = nullptr;
    QLabel* m_signal = nullptr;
    QLabel* m_stage = nullptr;
    QLabel* m_owner = nullptr;
    DataFlowCanvas* m_flow = nullptr;
    IntersectionTrendPlot* m_trend = nullptr;
    QTextBrowser* m_detail = nullptr;
    QTextBrowser* m_api = nullptr;
};
