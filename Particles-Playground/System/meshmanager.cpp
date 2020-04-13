#include "meshmanager.h"
#include "graphic.h"
#include "commandlist.h"
#include "fence.h"
#include "gpubuffer.h"

MeshManager::~MeshManager()
{ }

bool MeshManager::Startup()
{
    CommandList cmdList(QueueType::Direct);

    CreateSquare(cmdList);

    cmdList.Submit();

    return true;
}

bool MeshManager::Shutdown()
{
    for (MeshResource& mesh : mMeshes)
    {
        mesh.Release();
    }

    return true;
}

void MeshManager::Bind(CommandList& cmdList, MeshType type) const
{
    const MeshResource& mesh = mMeshes[static_cast<uint32_t>(type)];
    assert(mesh.VertexBuffer);
    cmdList->IASetVertexBuffers(0, 1, &mesh.VertexBufferView);
    cmdList->IASetPrimitiveTopology(mesh.Topology);
    cmdList->IASetIndexBuffer(&mesh.IndexBufferView);
}

void MeshManager::Draw(CommandList& cmdList, MeshType type, uint32_t instanceCount) const
{
    const MeshResource& mesh = mMeshes[static_cast<uint32_t>(type)];
    cmdList->DrawIndexedInstanced(mesh.Count, instanceCount, 0, 0, 0);
}

VertexFormatDescRef MeshManager::GetVertexFormatDescRef(MeshType type) const
{
    const MeshResource& mesh = mMeshes[static_cast<uint32_t>(type)];
    return mesh.VertexFormat;
}

void MeshManager::CreateSquare(CommandList& cmdList)
{
    DefaultVertex vertices[] =
    {
        { { -1.0f,  1.0f, 0.0f }, { 0.f, 0.f } },
        { {  1.0f, -1.0f, 0.0f }, { 1.f, 1.f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.f, 1.f } },
        { {  1.0f,  1.0f, 0.0f }, { 1.f, 0.f } }
    };

    uint32_t indices[] = { 0, 1, 2, 
                           0, 3, 1 
    };

    MeshResource& mesh = mMeshes[static_cast<uint32_t>(MeshType::Square)];

    mesh.Count = _countof(indices);
    mesh.VertexFormat = GetVertexFormatDesc<std::remove_all_extents_t<decltype(vertices)>>();
    
    const uint32_t verticesSize = sizeof(vertices);
    const uint32_t indicesSize = sizeof(indices);
    mesh.VertexBuffer = std::make_unique<GPUBuffer>(verticesSize, 1, BufferUsage::Vertex);
    mesh.IndexBuffer = std::make_unique<GPUBuffer>(indicesSize, 1, BufferUsage::Index);

    uint8_t* data = nullptr;

    // Vertex buffer
    data = mesh.VertexBuffer->Map(0, verticesSize);
    memcpy(data, &vertices, verticesSize);
    mesh.VertexBuffer->Unmap(cmdList);

    mesh.VertexBufferView.BufferLocation = mesh.VertexBuffer->GetGPUAddress();
    mesh.VertexBufferView.SizeInBytes = verticesSize;
    mesh.VertexBufferView.StrideInBytes = sizeof(DefaultVertex);

    // Index buffer
    data = mesh.IndexBuffer->Map(0, indicesSize);
    memcpy(data, &indices, indicesSize);
    mesh.IndexBuffer->Unmap(cmdList);
    
    mesh.IndexBufferView.BufferLocation = mesh.IndexBuffer->GetGPUAddress();
    mesh.IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mesh.IndexBufferView.SizeInBytes = indicesSize;

}
