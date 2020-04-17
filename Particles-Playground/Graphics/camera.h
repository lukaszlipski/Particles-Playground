#pragma once

class Camera
{
public:
    Camera(const XMVECTOR& pos, const XMVECTOR& dir, float fov = 90.0f, float nearDist = 1.0f, float farDist = 100.0f);

    XMMATRIX GetView() const;
    XMMATRIX GetProjection() const;

private:
    XMVECTOR mPosition;
    XMVECTOR mDirection;
    float mFov;
    float mNearDist;
    float mFarDist;

};
