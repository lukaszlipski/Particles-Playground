#pragma once
#include "System/engine.h"

class PresentToScreenNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        context.InputTexture2D(RESOURCEID("RenderTarget"), TextureUsage::ShaderResource);
    }

    void Execute(const RGExecuteContext& context) override;
};
