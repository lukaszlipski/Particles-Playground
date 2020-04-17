#include "camera.h"
#include "System/window.h"

Camera::Camera(const XMVECTOR& pos, const XMVECTOR& dir, float fov /*= 90.0f*/, float nearDist /*= 1.0f*/, float farDist /*= 100.0f*/) 
    : mPosition(pos), mDirection(dir), mFov(fov), mNearDist(nearDist), mFarDist(farDist)
{ }

XMMATRIX Camera::GetView() const
{
    const XMVECTOR camUp = { 0, 1, 0,0 };
    return XMMatrixLookAtLH(mPosition, mPosition + mDirection, camUp);
}

XMMATRIX Camera::GetProjection() const
{
    const float fov = XMConvertToRadians(90.0f);
    const float ratio = Window::Get().GetWidth() / static_cast<float>(Window::Get().GetHeight());
    return XMMatrixPerspectiveFovLH(fov, ratio, mNearDist, mFarDist);
}
