#pragma once
#include "vertexformats.h"

class CommandList;
class MeshManager;
class GPUBuffer;

enum class MeshType
{
    Square = 0,
    Max
};

struct MeshResource
{
    std::unique_ptr<GPUBuffer> VertexBuffer;
    std::unique_ptr<GPUBuffer> IndexBuffer;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;

    D3D_PRIMITIVE_TOPOLOGY Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    uint32_t Count = 0;

    VertexFormatDescRef VertexFormat = nullptr;

private:
    friend MeshManager;

    inline void Release()
    {
        VertexBuffer.reset();
        IndexBuffer.reset();
    }
};

class MeshManager
{
public:
    ~MeshManager();

    MeshManager(const MeshManager&) = delete;
    MeshManager(MeshManager&&) = delete;

    MeshManager& operator=(const MeshManager&) = delete;
    MeshManager& operator=(MeshManager&&) = delete;

    bool Startup();
    bool Shutdown();

    static MeshManager& Get()
    {
        static MeshManager* instance = new MeshManager();
        return *instance;
    }

    void Bind(CommandList& cmdList, MeshType type) const;
    void Draw(CommandList& cmdList, MeshType type, uint32_t instanceCount = 1) const;
    VertexFormatDescRef GetVertexFormatDescRef(MeshType type) const;

private:
    explicit MeshManager() = default;

    void CreateSquare(CommandList& cmdList);

    std::array<MeshResource, static_cast<uint32_t>(MeshType::Max)> mMeshes;

};
