#include "utils/RenderProfiler.h"

#include "core/opengl.h"

#include <algorithm>
#include <limits>

namespace {
constexpr std::size_t HistorySize = 120;
}

void RenderProfiler::BeginFrame()
{
    PollGpuQueries();
    if (!enabled || paused) return;

    for (auto& entry : records) entry.second.frameCpuMs = 0.0;
    frameStart = Clock::now();
    frameStartQuery = AcquireQuery();
    frameEndQuery = AcquireQuery();
    glQueryCounter(frameStartQuery, GL_TIMESTAMP);
    frameOpen = true;
}

void RenderProfiler::EndFrame()
{
    if (!frameOpen) return;

    frameCpuMs = std::chrono::duration<double, std::milli>(Clock::now() - frameStart).count();
    if (!discardCurrentFrame)
        PushHistory(frameCpuHistory, static_cast<float>(frameCpuMs));
    for (auto& entry : records) {
        Record& record = entry.second;
        if (!discardCurrentFrame && record.frameCpuMs > 0.0) {
            record.latestCpuMs = record.frameCpuMs;
            record.cpuTotalMs += record.frameCpuMs;
            ++record.cpuSamples;
            PushHistory(record.cpuHistory, static_cast<float>(record.frameCpuMs));
        }
        record.frameCpuMs = 0.0;
    }

    glQueryCounter(frameEndQuery, GL_TIMESTAMP);
    if (!discardCurrentFrame) GetOrCreateRecord("Top-level GPU").pendingQueries++;
    pendingQueries.push_back({ frameStartQuery, frameEndQuery, "Top-level GPU", discardCurrentFrame });
    frameStartQuery = 0;
    frameEndQuery = 0;
    frameOpen = false;
    discardCurrentFrame = false;
}

RenderProfiler::ScopeToken RenderProfiler::BeginPass(const char* name)
{
    ScopeToken token;
    if (!enabled || paused || name == nullptr || *name == '\0') return token;

    token.name = scopeStack.empty() ? name : scopeStack.back() + "/" + name;
    scopeStack.push_back(token.name);
    token.cpuStart = Clock::now();
    token.startQuery = AcquireQuery();
    token.endQuery = AcquireQuery();
    glQueryCounter(token.startQuery, GL_TIMESTAMP);
    token.active = true;
    GetOrCreateRecord(token.name).pendingQueries++;
    return token;
}

void RenderProfiler::EndPass(ScopeToken& token)
{
    if (!token.active) return;

    glQueryCounter(token.endQuery, GL_TIMESTAMP);
    const double cpuMs = std::chrono::duration<double, std::milli>(Clock::now() - token.cpuStart).count();
    Record& record = GetOrCreateRecord(token.name);
    record.frameCpuMs += cpuMs;
    if (!frameOpen) {
        record.latestCpuMs = cpuMs;
        record.cpuTotalMs += cpuMs;
        ++record.cpuSamples;
        PushHistory(record.cpuHistory, static_cast<float>(cpuMs));
        record.frameCpuMs = 0.0;
    }
    const bool discard = discardCurrentFrame && token.name.rfind("One-time/", 0) != 0;
    pendingQueries.push_back({ token.startQuery, token.endQuery, token.name, discard });

    if (!scopeStack.empty()) scopeStack.pop_back();
    token.active = false;
}

void RenderProfiler::SetEnabled(bool value)
{
    if (enabled == value) return;
    enabled = value;
}

void RenderProfiler::SetPaused(bool value)
{
    if (paused == value) return;
    paused = value;
}

unsigned int RenderProfiler::AcquireQuery()
{
    if (!freeQueries.empty()) {
        const unsigned int query = freeQueries.back();
        freeQueries.pop_back();
        return query;
    }
    unsigned int query = 0;
    glGenQueries(1, &query);
    return query;
}

void RenderProfiler::ReleaseQuery(unsigned int query)
{
    if (query != 0) freeQueries.push_back(query);
}

void RenderProfiler::PollGpuQueries()
{
    for (auto it = pendingQueries.begin(); it != pendingQueries.end();) {
        GLint available = GL_FALSE;
        glGetQueryObjectiv(it->end, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available == GL_FALSE) {
            ++it;
            continue;
        }

        GLuint64 start = 0;
        GLuint64 end = 0;
        glGetQueryObjectui64v(it->start, GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(it->end, GL_QUERY_RESULT, &end);

        if (it->discard) {
            ReleaseQuery(it->start);
            ReleaseQuery(it->end);
            it = pendingQueries.erase(it);
            continue;
        }
        const double gpuMs = end >= start ? static_cast<double>(end - start) / 1000000.0 : 0.0;

        Record& record = GetOrCreateRecord(it->passName);
        record.latestGpuMs = gpuMs;
        if (it->passName == "Top-level GPU") frameGpuMs = gpuMs;
        record.gpuTotalMs += gpuMs;
        record.minGpuMs = record.gpuSamples == 0 ? gpuMs : std::min(record.minGpuMs, gpuMs);
        record.maxGpuMs = record.gpuSamples == 0 ? gpuMs : std::max(record.maxGpuMs, gpuMs);
        ++record.gpuSamples;
        if (record.pendingQueries > 0) --record.pendingQueries;
        PushHistory(record.gpuHistory, static_cast<float>(gpuMs));

        ReleaseQuery(it->start);
        ReleaseQuery(it->end);
        it = pendingQueries.erase(it);
    }
}

RenderProfiler::Record& RenderProfiler::GetOrCreateRecord(const std::string& name)
{
    auto result = records.emplace(name, Record{});
    if (result.second) result.first->second.name = name;
    return result.first->second;
}

void RenderProfiler::PushHistory(std::deque<float>& history, float value)
{
    history.push_back(value);
    if (history.size() > HistorySize) history.pop_front();
}

std::vector<RenderProfiler::PassStats> RenderProfiler::GetStats() const
{
    std::vector<PassStats> result;
    result.reserve(records.size());
    for (const auto& entry : records) {
        const Record& record = entry.second;
        PassStats stats;
        stats.name = record.name;
        stats.cpuMs = record.latestCpuMs;
        stats.gpuMs = record.latestGpuMs;
        stats.averageCpuMs = record.cpuSamples ? record.cpuTotalMs / record.cpuSamples : 0.0;
        stats.averageGpuMs = record.gpuSamples ? record.gpuTotalMs / record.gpuSamples : 0.0;
        stats.minGpuMs = record.minGpuMs;
        stats.maxGpuMs = record.maxGpuMs;
        stats.cpuSamples = record.cpuSamples;
        stats.gpuSamples = record.gpuSamples;
        stats.gpuPending = record.pendingQueries > 0;
        stats.cpuHistory.assign(record.cpuHistory.begin(), record.cpuHistory.end());
        stats.gpuHistory.assign(record.gpuHistory.begin(), record.gpuHistory.end());
        result.push_back(std::move(stats));
    }
    std::sort(result.begin(), result.end(), [](const PassStats& a, const PassStats& b) {
        return a.name < b.name;
    });
    return result;
}

double RenderProfiler::GetAverageFrameCpuMs() const
{
    if (frameCpuHistory.empty()) return 0.0;
    double total = 0.0;
    for (float value : frameCpuHistory) total += value;
    return total / static_cast<double>(frameCpuHistory.size());
}

double RenderProfiler::GetAverageFrameGpuMs() const
{
    const auto found = records.find("Top-level GPU");
    if (found == records.end() || found->second.gpuSamples == 0) return 0.0;
    return found->second.gpuTotalMs / static_cast<double>(found->second.gpuSamples);
}

void RenderProfiler::Reset()
{
    records.clear();
    for (PendingQuery& pending : pendingQueries) pending.discard = true;
    discardCurrentFrame = frameOpen;
    frameCpuMs = 0.0;
    frameGpuMs = 0.0;
    frameCpuHistory.clear();
}

void RenderProfiler::ResetFrameHistory()
{
    for (auto it = records.begin(); it != records.end();) {
        if (it->first.rfind("One-time/", 0) == 0) ++it;
        else it = records.erase(it);
    }
    for (PendingQuery& pending : pendingQueries) {
        if (pending.passName.rfind("One-time/", 0) != 0)
            pending.discard = true;
    }
    discardCurrentFrame = frameOpen;
    frameCpuMs = 0.0;
    frameGpuMs = 0.0;
    frameCpuHistory.clear();
}

void RenderProfiler::Shutdown()
{
    std::vector<unsigned int> queries = freeQueries;
    for (const PendingQuery& pending : pendingQueries) {
        queries.push_back(pending.start);
        queries.push_back(pending.end);
    }
    if (!queries.empty()) glDeleteQueries(static_cast<GLsizei>(queries.size()), queries.data());
    freeQueries.clear();
    pendingQueries.clear();
    scopeStack.clear();
    records.clear();
    frameStartQuery = 0;
    frameEndQuery = 0;
    frameCpuHistory.clear();
    frameOpen = false;
}

RenderProfileScope::RenderProfileScope(RenderProfiler& profiler, const char* name)
    : profiler(profiler), token(profiler.BeginPass(name))
{
}

RenderProfileScope::~RenderProfileScope()
{
    profiler.EndPass(token);
}
