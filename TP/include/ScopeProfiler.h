#pragma once

#include <chrono>
#include <functional>

class ScopeProfiler
{
public:

    ScopeProfiler(const std::function<void(float)>& endCallback)
        : m_EndCallback(endCallback)
    {
        m_StartTime = std::chrono::high_resolution_clock::now();
    }

    ~ScopeProfiler()
    {
        float duration = getDuration();
        m_EndCallback(duration);
    }

    float getDuration() const
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - m_StartTime).count() * 1e-9f;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    std::function<void(float)> m_EndCallback;
};

#define PROFILE_SCOPE_VARIABLE(var) ScopeProfiler profile_##var([&, this](float duration) { var = duration; })