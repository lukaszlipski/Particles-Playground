#pragma once

class CommandList;
class MeshManager;

enum class MeshType
{
    Square = 0,
    Max
};

struct MeshResource
{
    ID3D12Resource* VertexBuffer = nullptr;
    ID3D12Resource* IndexBuffer = nullptr;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;

    D3D_PRIMITIVE_TOPOLOGY Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    uint32_t Count = 0;

private:
    friend MeshManager;

    inline void Release()
    {
        if (VertexBuffer) { VertexBuffer->Release(); }
        if (IndexBuffer) { IndexBuffer->Release(); }
    }
};

class MeshManager
{
public:
    MeshManager(const MeshManager&) = delete;
    MeshManager(MeshManager&&) = delete;

    MeshManager& operator=(const MeshManager&) = delete;
    MeshManager& operator=(MeshManager&&) = delete;

    bool Startup();
    bool Shutdown();

    void PostInit();

    static MeshManager& Get()
    {
        static MeshManager* instance = new MeshManager();
        return *instance;
    }

    void Bind(CommandList& cmdList, MeshType type) const;
    void Draw(CommandList& cmdList, MeshType type, uint32_t instanceCount = 1) const;

private:
    explicit MeshManager() = default;

    void CreateSquare(CommandList& copyCmdList, CommandList& postCopyCmdList);

    std::unique_ptr<class Fence> mCopyFence;
    std::array<MeshResource, static_cast<uint32_t>(MeshType::Max)> mMeshes;
    std::vector<ID3D12Resource*> mStageBuffers;

};
