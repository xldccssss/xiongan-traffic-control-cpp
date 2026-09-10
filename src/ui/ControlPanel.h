#pragma once

#include <QList>
#include <QString>
#include <QWidget>

class QLabel;
class QPushButton;
class QSpinBox;

class ControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ControlPanel(int vehicleSeed,
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
                          QWidget* parent = nullptr);

public slots:
    void updateComparisonState(bool active,
                               int currentRound,
                               int totalRounds,
                               const QString& algorithm,
                               int vehicleSeed,
                               int destinationSeed,
                               int resultRound,
                               const QString& status);

signals:
    void startRequested();
    void pauseRequested();
    void resetRequested();
    void stopRequested();
    void algorithmChanged(const QString& algorithm);
    void speedChanged(int multiple);
    void outerRateChanged(int vehiclesPerHour);
    void residentialRateChanged(int vehiclesPerHour);
    void durationChanged(int seconds);
    void randomSeedsChanged(int vehicleSeed, int destinationSeed);
    void preloadCountChanged(int count);
    void comparisonRequested(int rounds);
    void comparisonCancelRequested();

private:
    void refreshDemandPreview();
    void selectAlgorithm(const QString& algorithm);

    QPushButton* m_fixedTiming = nullptr;
    QPushButton* m_maxPressure = nullptr;
    QLabel* m_algorithmHint = nullptr;
    QSpinBox* m_speed = nullptr;
    QSpinBox* m_outerRate = nullptr;
    QSpinBox* m_residentialRate = nullptr;
    QSpinBox* m_duration = nullptr;
    QSpinBox* m_vehicleSeed = nullptr;
    QSpinBox* m_destinationSeed = nullptr;
    QSpinBox* m_preloadCount = nullptr;
    QSpinBox* m_comparisonRounds = nullptr;
    QPushButton* m_comparisonStart = nullptr;
    QPushButton* m_comparisonCancel = nullptr;
    QPushButton* m_reset = nullptr;
    QLabel* m_seedHint = nullptr;
    QLabel* m_comparisonHint = nullptr;
    QLabel* m_outerSubtotal = nullptr;
    QLabel* m_residentialSubtotal = nullptr;
    QLabel* m_totalDemand = nullptr;
    int m_outerEntryCount = 0;
    int m_residentialEntryCount = 0;
    QList<QWidget*> m_comparisonLockedWidgets;
};
