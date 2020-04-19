#include "System/psomanager.h"
#include "System/graphic.h"
#include "System/commandlist.h"
#include "System/pipelinestate.h"
#include "System/shaderparameterslayout.h"
#include "Utilities/debug.h"

const std::wstring SHADER_FOLDER = L"Shaders/";

bool PSOManager::Startup()
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature{};
    mRootSigVer = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(Graphic::Get().GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature, sizeof(feature))))
    {
        mRootSigVer = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    return true;
}

bool PSOManager::Shutdown()
{
    for (auto& [key, shader] : mShaders)
    {
        shader->Release();
    }

    for (auto& [key, pso] : mCachedPipelineStates)
    {
        pso->Release();
    }

    for (auto& [type, rootSig] : mCachedRootSignatures)
    {
        rootSig->Release();
    }

    return true;
}

ID3DBlob* PSOManager::GetShader(std::wstring_view name)
{
    const size_t key = std::hash<std::wstring_view>{}(name);
    auto shaderIt = mShaders.find(key);

    if (shaderIt != mShaders.end())
    {
        return shaderIt->second;
    }

    ID3DBlob* shader = nullptr;
    const std::wstring fileName = SHADER_FOLDER + name.data() + L".cso";
    HRESULT hr = D3DReadFileToBlob(fileName.c_str(), &shader);
    assert(SUCCEEDED(hr));
    mShaders[key] = shader;

    return shader;
}

ID3D12PipelineState* PSOManager::CompilePipelineState(const GraphicPipelineState& pipelineState)
{
    const uint32_t key = pipelineState.Hash();
    auto psoIt = mCachedPipelineStates.find(key);

    if (psoIt != mCachedPipelineStates.end())
    {
        return psoIt->second;
    }

    ID3D12Device* device = Graphic::Get().GetDevice();

    ID3D12PipelineState* pso = nullptr;
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc = pipelineState;
    HRESULT hr = device->CreateGraphicsPipelineState(&stateDesc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr));

    mCachedPipelineStates[key] = pso;
    return pso;
}

ID3D12RootSignature* PSOManager::CompileShaderParameterLayout(const ShaderParametersLayout& layout)
{
    const uint32_t key = layout.Hash();
    auto layoutIt = mCachedRootSignatures.find(key);

    if (layoutIt != mCachedRootSignatures.end())
    {
        return layoutIt->second;
    }

    RootParameters params = layout.GetParameters();
    auto& [rootParams, ranges] = params;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.Init_1_1(static_cast<uint32_t>(rootParams.size()), rootParams.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob* blob = nullptr;
    ID3DBlob* error = nullptr;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSigDesc, mRootSigVer, &blob, &error);

    if (FAILED(hr))
    {
        const char* errorMsg = reinterpret_cast<const char*>(error->GetBufferPointer());
        OutputDebugMessage("Root Signature Error: %s\n", errorMsg);
        assert(0);
    }

    ID3D12Device* device = Graphic::Get().GetDevice();

    ID3D12RootSignature* rootSig = nullptr;
    hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
    assert(SUCCEEDED(hr));
    blob->Release();

    mCachedRootSignatures[key] = rootSig;
    return rootSig;
}
