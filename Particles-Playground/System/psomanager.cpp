#include "psomanager.h"
#include "graphic.h"
#include "CommandList.h"

const std::wstring SHADER_FOLDER = L"Shaders/";

bool PSOManager::Startup()
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature{};
    mRootSigVer = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(Graphic::Get().GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature, sizeof(feature))))
    {
        mRootSigVer = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Prepare Root Signatures
    SetupDefaultRootSig();

    // Prepare PSOs
    SetupDefaultPSO();

    return true;
}

bool PSOManager::Shutdown()
{
    for (auto& [type, extendedPSO] : mPSOMap)
    {
        auto& [pso, rootSigType, pipelineType] = extendedPSO;
        pso->Release();
    }

    for (auto& [type, rootSig] : mRootSigMap)
    {
        rootSig->Release();
    }

    return true;
}

void PSOManager::Bind(CommandList& cmdList, const PSOType type)
{
    auto& [pso, rootSigType, pipelineType] = mPSOMap[type];
    assert(pso);
    ID3D12RootSignature* rootSig = mRootSigMap[rootSigType];
    assert(rootSig);

    cmdList->SetPipelineState(pso);
    
    switch (pipelineType)
    {
    case PipelineType::Graphics: { cmdList->SetGraphicsRootSignature(rootSig); break; }
    case PipelineType::Compute: { cmdList->SetComputeRootSignature(rootSig); break; }
    }
}

bool PSOManager::SetupDefaultRootSig()
{  
    CD3DX12_ROOT_PARAMETER1 rootParams[2];
    rootParams[0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParams[1].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS);

    ID3DBlob* blob = nullptr;
    ID3DBlob* error = nullptr;
    D3DX12SerializeVersionedRootSignature(&rootSigDesc, mRootSigVer, &blob, &error);

    ID3D12RootSignature* rootSig = nullptr;
    if (FAILED(Graphic::Get().GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig))))
    {
        if (blob) { blob->Release(); }
        if (error) { error->Release(); }

        return false;
    }

    if (blob) { blob->Release(); }
    if (error) { error->Release(); }

    mRootSigMap[RootSigType::Default] = rootSig;

    return true;
}

bool PSOManager::SetupDefaultPSO()
{
    const RootSigType rootSig = RootSigType::Default;

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    std::wstring vsShaderFileName = SHADER_FOLDER + L"vsdefault.cso";
    std::wstring psShaderFileName = SHADER_FOLDER + L"psdefault.cso";
    
    D3DReadFileToBlob(vsShaderFileName.c_str(), &vertexShader);
    D3DReadFileToBlob(psShaderFileName.c_str(), &pixelShader);

    std::array<D3D12_INPUT_ELEMENT_DESC, 1> inputs = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC info{};
    info.pRootSignature = mRootSigMap[rootSig];
    info.InputLayout = { inputs.data() , static_cast<uint32_t>(inputs.size()) };
    info.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
    info.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
    //info.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    info.NumRenderTargets = 1;
    info.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    info.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    info.SampleMask = UINT_MAX;
    info.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    info.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    info.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    info.SampleDesc.Count = 1;

    ID3D12PipelineState* pso = nullptr;
    if (FAILED(Graphic::Get().GetDevice()->CreateGraphicsPipelineState(&info, IID_PPV_ARGS(&pso))))
    {
        if (vertexShader) { vertexShader->Release(); }
        if (pixelShader) { pixelShader->Release(); }
        return false;
    }

    if (vertexShader) { vertexShader->Release(); }
    if (pixelShader) { pixelShader->Release(); }

    mPSOMap[PSOType::Default] = { pso, rootSig, PipelineType::Graphics };
    return true;
}
