#pragma once

class GlobalTimer
{
public:
    GlobalTimer(float scale = 1.0f);

    void Reset();
    void PostUpdate();

    inline float GetDeltaTime() const { return mDeltaTimeMS * mScale; }
    inline float GetRawDeltaTime() const { return mDeltaTimeMS; }
    inline float GetElapsedTime() const { return mElapsedTime; }
    inline void SetScale(float scale) { mScale = scale; }

    float GetElapsedTimeRaw() const;

private:
    float mScale;
    float mDeltaTimeMS = 0.0f;
    float mElapsedTime = 0.0f;
    std::chrono::high_resolution_clock::time_point mStart;
    std::chrono::high_resolution_clock::time_point mLastUpdate;

};
