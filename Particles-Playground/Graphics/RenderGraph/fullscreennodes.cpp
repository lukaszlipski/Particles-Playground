#include "Graphics/RenderGraph/fullscreennodes.h"

void PresentToScreenNode::Execute(const RGExecuteContext& context)
{
    Texture2D* renderTarget = context.GetTexture2D(RESOURCEID("RenderTarget"));

    CommandList& commandList = context.GetCommandList();

    {
        std::array< CD3DX12_RESOURCE_BARRIER, 1> barriers;
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }

    // Render on screen
    Sampler defaultSampler;

    ShaderParametersLayout screenLayout;
    screenLayout.SetSRV(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    screenLayout.SetStaticSampler(0, defaultSampler, D3D12_SHADER_VISIBILITY_PIXEL);

    GraphicPipelineState screenState;
    screenState.SetVS(VS_Screen);
    screenState.SetPS(PS_Screen);
    screenState.Bind(commandList, screenLayout);

    ShaderParameters screenParams;
    screenParams.SetSRV(0, *renderTarget);
    screenParams.Bind<true>(commandList, screenLayout);

    MeshManager::Get().Bind(commandList, MeshType::Square);

    const CD3DX12_CPU_DESCRIPTOR_HANDLE rtHandle = Graphic::Get().GetCurrentRenderTargetHandle();
    commandList->OMSetRenderTargets(1, &rtHandle, false, nullptr);

    MeshManager::Get().Draw(commandList, MeshType::Square, 1);

    {
        std::array< CD3DX12_RESOURCE_BARRIER, 1> barriers;
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }
}
