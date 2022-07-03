#pragma once
#include "System/engine.h"

struct SceneContants
{
    XMMATRIX Projection;
    XMMATRIX View;
};

class PrepareSceneBufferNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        RGNewGPUBuffer& newBuffer = context.OutputGPUBuffer(RESOURCEID("SceneBuffer"), BufferUsage::CopyDst);
        newBuffer.mElemSize = sizeof(SceneContants);
        newBuffer.mNumElems = 1;
        newBuffer.mUsage = BufferUsage::Structured | BufferUsage::CopyDst;
    }

    void Execute(const RGExecuteContext& context) override;
};
