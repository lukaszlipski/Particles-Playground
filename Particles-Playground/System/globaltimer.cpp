#include "globaltimer.h"

GlobalTimer::GlobalTimer(float scale /*= 1.0f*/) : mScale(scale)
{
    Reset();
}

void GlobalTimer::Reset()
{
    mStart = std::chrono::high_resolution_clock::now();
    mLastUpdate = mStart;
    mDeltaTimeMS = 0.0f;
    mElapsedTime = 0.0f;
}

void GlobalTimer::PostUpdate()
{
    std::chrono::high_resolution_clock::time_point current = std::chrono::high_resolution_clock::now();
    mDeltaTimeMS = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(current - mLastUpdate).count());
    mDeltaTimeMS /= 1000.0f;
    mLastUpdate = current;
    mElapsedTime += mDeltaTimeMS;
}

float GlobalTimer::GetElapsedTimeRaw() const
{
    return static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(mStart - mLastUpdate).count());
}
