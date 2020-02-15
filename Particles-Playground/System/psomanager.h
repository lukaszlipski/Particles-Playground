#pragma 

class CommandList;

enum class PSOType
{
    Default = 0,
};

enum class RootSigType
{
    Default = 0,
};

enum class PipelineType
{
    Graphics = 0,
    Compute
};

class PSOManager
{
    using ExtendedPSO = std::tuple<ID3D12PipelineState*, RootSigType, PipelineType>;

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

    void Bind(CommandList& cmdList, const PSOType type);

private:
    explicit PSOManager() = default;

    bool SetupDefaultRootSig();
    bool SetupDefaultPSO();

    D3D_ROOT_SIGNATURE_VERSION mRootSigVer;
    std::map<RootSigType, ID3D12RootSignature*> mRootSigMap;
    std::map<PSOType, ExtendedPSO> mPSOMap;

};
