#pragma once

#include <QVariantMap>
#include <QVector>
#include <QWidget>

class QLabel;
class QPlainTextEdit;

class TrendPlot : public QWidget
{
    Q_OBJECT
public:
    explicit TrendPlot(QWidget* parent = nullptr);
    void append(double activeVehicles, double queueVehicles,
                double speedKph, double energyKWh);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_activeVehicles;
    QVector<double> m_queueVehicles;
    QVector<double> m_speed;
    QVector<double> m_energy;
};

class TrajectoryPlot : public QWidget
{
    Q_OBJECT
public:
    explicit TrajectoryPlot(QWidget* parent = nullptr);
    void setTrajectories(const QVariantMap& trajectories);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVariantMap m_trajectories;
};

class TelemetryPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TelemetryPanel(QWidget* parent = nullptr);

public slots:
    void updateTelemetry(const QVariantMap& values);
    void appendEvent(const QString& text);
    void resetView();

private:
    void setMetricColor(QLabel* label, const QString& color);

    QLabel* m_statusLine = nullptr;
    QLabel* m_time = nullptr;
    QLabel* m_activeVehicles = nullptr;
    QLabel* m_queue = nullptr;
    QLabel* m_speed = nullptr;
    QLabel* m_energy = nullptr;
    QLabel* m_completedDelay = nullptr;
    QLabel* m_completedVehicles = nullptr;
    QLabel* m_actualMultiple = nullptr;
    TrendPlot* m_plot = nullptr;
    TrajectoryPlot* m_trajectoryPlot = nullptr;
    QPlainTextEdit* m_events = nullptr;
};
