#pragma once

#include <cstdint>

enum class ManualResetAction
{
    ReadyStopped,
    RequestStop,
    WaitForStopCallback
};

// Keeps the plug-in lifecycle state authoritative while TESSNG is stopping.
// isRunning() can become false before CustomerSimulator::afterStop() arrives;
// starting in that gap is ignored by the native simulator.
class SimulationRestartGuard
{
public:
    ManualResetAction requestReset(bool lifecycleRunning,
                                   bool simulatorRunningOrPaused)
    {
        m_pending = true;
        m_startRequested = false;
        ++m_generation;
        if (simulatorRunningOrPaused) return ManualResetAction::RequestStop;
        if (lifecycleRunning) return ManualResetAction::WaitForStopCallback;
        return ManualResetAction::ReadyStopped;
    }

    bool pending() const { return m_pending; }
    bool startRequested() const { return m_startRequested; }
    void requestStart()
    {
        if (m_pending) m_startRequested = true;
    }
    std::uint64_t generation() const { return m_generation; }
    bool isCurrent(std::uint64_t generation) const
    {
        return m_pending && generation == m_generation;
    }

    void startAccepted()
    {
        m_pending = false;
        m_startRequested = false;
    }

    void cancel()
    {
        m_pending = false;
        m_startRequested = false;
        ++m_generation;
    }

private:
    bool m_pending = false;
    bool m_startRequested = false;
    std::uint64_t m_generation = 0;
};
