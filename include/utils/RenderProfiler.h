#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

class RenderProfiler
{
public:
    RenderProfiler() = default;
    RenderProfiler(const RenderProfiler&) = delete;
    RenderProfiler& operator=(const RenderProfiler&) = delete;

    struct PassStats
    {
        std::string name;
        double cpuMs = 0.0;
        double gpuMs = 0.0;
        double averageCpuMs = 0.0;
        double averageGpuMs = 0.0;
        double minGpuMs = 0.0;
        double maxGpuMs = 0.0;
        std::uint64_t cpuSamples = 0;
        std::uint64_t gpuSamples = 0;
        bool gpuPending = false;
        std::vector<float> cpuHistory;
        std::vector<float> gpuHistory;
    };

    void BeginFrame();
    void EndFrame();
    void Shutdown();
    void Reset();
    void ResetFrameHistory();

    void SetEnabled(bool value);
    bool IsEnabled() const { return enabled; }
    void SetPaused(bool value);
    bool IsPaused() const { return paused; }

    std::vector<PassStats> GetStats() const;
    double GetFrameCpuMs() const { return frameCpuMs; }
    double GetFrameGpuMs() const { return frameGpuMs; }
    double GetAverageFrameCpuMs() const;
    double GetAverageFrameGpuMs() const;

private:
    friend class RenderProfileScope;

    using Clock = std::chrono::steady_clock;

    struct Record
    {
        std::string name;
        double frameCpuMs = 0.0;
        double latestCpuMs = 0.0;
        double latestGpuMs = 0.0;
        double cpuTotalMs = 0.0;
        double gpuTotalMs = 0.0;
        double minGpuMs = 0.0;
        double maxGpuMs = 0.0;
        std::uint64_t cpuSamples = 0;
        std::uint64_t gpuSamples = 0;
        std::size_t pendingQueries = 0;
        std::deque<float> cpuHistory;
        std::deque<float> gpuHistory;
    };

    struct PendingQuery
    {
        unsigned int start = 0;
        unsigned int end = 0;
        std::string passName;
        bool discard = false;
    };

    struct ScopeToken
    {
        std::string name;
        Clock::time_point cpuStart;
        unsigned int startQuery = 0;
        unsigned int endQuery = 0;
        bool active = false;
    };

    ScopeToken BeginPass(const char* name);
    void EndPass(ScopeToken& token);
    unsigned int AcquireQuery();
    void ReleaseQuery(unsigned int query);
    void PollGpuQueries();
    Record& GetOrCreateRecord(const std::string& name);
    static void PushHistory(std::deque<float>& history, float value);

    bool enabled = true;
    bool paused = false;
    bool frameOpen = false;
    bool discardCurrentFrame = false;
    Clock::time_point frameStart{};
    double frameCpuMs = 0.0;
    double frameGpuMs = 0.0;
    unsigned int frameStartQuery = 0;
    unsigned int frameEndQuery = 0;
    std::deque<float> frameCpuHistory;
    std::vector<std::string> scopeStack;
    std::unordered_map<std::string, Record> records;
    std::deque<PendingQuery> pendingQueries;
    std::vector<unsigned int> freeQueries;
};

class RenderProfileScope
{
public:
    RenderProfileScope(RenderProfiler& profiler, const char* name);
    ~RenderProfileScope();

    RenderProfileScope(const RenderProfileScope&) = delete;
    RenderProfileScope& operator=(const RenderProfileScope&) = delete;

private:
    RenderProfiler& profiler;
    RenderProfiler::ScopeToken token;
};

#define A3_PROFILE_CONCAT_IMPL(a, b) a##b
#define A3_PROFILE_CONCAT(a, b) A3_PROFILE_CONCAT_IMPL(a, b)
#define A3_PROFILE_PASS(profiler, name) RenderProfileScope A3_PROFILE_CONCAT(a3ProfileScope_, __LINE__)(profiler, name)
