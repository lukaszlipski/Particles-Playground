#include "System/pipelinestate.h"
#include "System/psomanager.h"
#include "System/commandlist.h"
#include "System/shaderparameterslayout.h"
#include "System/window.h"

GraphicPipelineState::GraphicPipelineState()
{
    const float width = static_cast<float>(Window::Get().GetWidth());
    const float height = static_cast<float>(Window::Get().GetHeight());

    for (CD3DX12_VIEWPORT& viewport : mViewports)
    {
        viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, width, height);
    }

    for (CD3DX12_RECT& rect : mScissorRects)
    {
        rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    }

    mState.pRootSignature = nullptr;

    SetVertexFormat<DefaultVertex>();
    SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    SetRTNum(1);
    SetRTFormat(0, DXGI_FORMAT_R8G8B8A8_UNORM);

    mState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    mState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    //mState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    //mState.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    mState.SampleMask = UINT_MAX;
    mState.SampleDesc.Count = 1;
}

GraphicPipelineState& GraphicPipelineState::SetVS(ShaderHandle handle)
{
    Shader* shader = ShaderManager::Get().GetShader(handle);
    Assert(shader && shader->GetType() == ShaderType::Vertex);
    mState.VS.pShaderBytecode = shader->GetBlob()->GetBufferPointer();
    mState.VS.BytecodeLength = shader->GetBlob()->GetBufferSize();
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetPS(ShaderHandle handle)
{
    Shader* shader = ShaderManager::Get().GetShader(handle);
    Assert(shader && shader->GetType() == ShaderType::Pixel);
    mState.PS.pShaderBytecode = shader->GetBlob()->GetBufferPointer();
    mState.PS.BytecodeLength = shader->GetBlob()->GetBufferSize();
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetRTNum(uint32_t num)
{
    mState.NumRenderTargets = num;
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
    mState.PrimitiveTopologyType = type;
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetRTFormat(uint32_t idx, DXGI_FORMAT format)
{
    Assert(idx < 8);
    mState.RTVFormats[idx] = format;
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetDSFormat(DXGI_FORMAT format)
{
    mState.DSVFormat = format;
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetDSState(const D3D12_DEPTH_STENCIL_DESC& state)
{
    mState.DepthStencilState = state;
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetRTBlendState(uint32_t idx, const D3D12_RENDER_TARGET_BLEND_DESC& blend)
{
    Assert(idx < 8);
    mState.BlendState.RenderTarget[idx] = blend;
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetIndependentBlend(bool enable)
{
    mState.BlendState.IndependentBlendEnable = enable;
    return *this;
}

GraphicPipelineState& GraphicPipelineState::SetViewportProperties(uint32_t idx, const CD3DX12_VIEWPORT& properties)
{
    Assert(idx < 8);
    mViewports[idx] = properties;
    return *this;
}

void GraphicPipelineState::Bind(CommandList& commandList, ShaderParametersLayout& layout)
{
    PSOManager& psoManager = PSOManager::Get();

    // Get and set root signature based on shader parameters layout
    ID3D12RootSignature* rootSig = psoManager.CompileShaderParameterLayout(layout);
    commandList->SetGraphicsRootSignature(rootSig);

    // Update state's sturcture with a proper root signature
    mState.pRootSignature = rootSig;

    // Get and set pipeline state based on provided parameters
    ID3D12PipelineState* pso = PSOManager::Get().CompilePipelineState(*this);
    commandList->SetPipelineState(pso);

    // Set viewports' properties
    commandList->RSSetViewports(mState.NumRenderTargets, mViewports.data());
    commandList->RSSetScissorRects(mState.NumRenderTargets, mScissorRects.data());

}

ComputePipelineState::ComputePipelineState()
{
}

ComputePipelineState& ComputePipelineState::SetCS(ShaderHandle handle)
{
    Shader* shader = ShaderManager::Get().GetShader(handle);
    Assert(shader && shader->GetType() == ShaderType::Compute);
    mState.CS.pShaderBytecode = shader->GetBlob()->GetBufferPointer();
    mState.CS.BytecodeLength = shader->GetBlob()->GetBufferSize();
    return *this;
}

void ComputePipelineState::Bind(CommandList& commandList, ShaderParametersLayout& layout)
{
    PSOManager& psoManager = PSOManager::Get();

    // Get and set root signature based on shader parameters layout
    ID3D12RootSignature* rootSig = psoManager.CompileShaderParameterLayout(layout);
    commandList->SetComputeRootSignature(rootSig);

    // Update state's structure with a proper root signature
    mState.pRootSignature = rootSig;

    // Get and set pipeline state based on provided parameters
    ID3D12PipelineState* pso = PSOManager::Get().CompilePipelineState(*this);
    commandList->SetPipelineState(pso);
}
