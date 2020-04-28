#pragma once

class CommandList;
class ShaderParametersLayout;
class GraphicPipelineState;
class ComputePipelineState;

enum class RootSigType
{
    Default = 0,
};

class PSOManager
{
public:
    PSOManager(const PSOManager&) = delete;
    PSOManager(PSOManager&&) = delete;

    PSOManager& operator=(const PSOManager&) = delete;
    PSOManager& operator=(PSOManager&&) = delete;

    bool Startup();
    bool Shutdown();

    static PSOManager& Get()
    {
        static PSOManager* instance = new PSOManager();
        return *instance;
    }

    ID3DBlob* GetShader(std::wstring_view name);
    template<typename PipelineState>
    ID3D12PipelineState* CompilePipelineState(const PipelineState& pipelineState);
    ID3D12RootSignature* CompileShaderParameterLayout(const ShaderParametersLayout& layout);

private:
    explicit PSOManager() = default;

    ID3D12PipelineState* CreatePipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
    ID3D12PipelineState* CreatePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);

    D3D_ROOT_SIGNATURE_VERSION mRootSigVer = D3D_ROOT_SIGNATURE_VERSION_1_1;
    std::map<size_t, ID3DBlob*> mShaders;
    std::map<uint32_t, ID3D12RootSignature*> mCachedRootSignatures;
    std::map<uint32_t, ID3D12PipelineState*> mCachedPipelineStates;

};

