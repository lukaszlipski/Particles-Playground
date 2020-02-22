#include "meshmanager.h"
#include "vertexformats.h"
#include "graphic.h"
#include "commandlist.h"
#include "fence.h"

bool MeshManager::Startup()
{
    mCopyFence = std::make_unique<Fence>();

    CommandList copyCmdList(QueueType::Copy);
    CommandList postCopyCmdList(QueueType::Direct);

    CreateSquare(copyCmdList, postCopyCmdList);

    copyCmdList.Submit();

    mCopyFence->Signal(QueueType::Copy);
    mCopyFence->Wait(QueueType::Direct);

    postCopyCmdList.Submit();

    return true;
}

bool MeshManager::Shutdown()
{
    mCopyFence.reset();

    for (MeshResource& mesh : mMeshes)
    {
        mesh.Release();
    }

    return true;
}

void MeshManager::PostStartup()
{
    mCopyFence->Flush(QueueType::Direct);

    for (ID3D12Resource* resource : mStageBuffers)
    {
        resource->Release();
    }
    mStageBuffers.clear();
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

void MeshManager::CreateSquare(CommandList& copyCmdList, CommandList& postCopyCmdList)
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

    ID3D12Resource* vertexStageBuff = nullptr;
    ID3D12Resource* indexStageBuff = nullptr;

    // Vertex buffer
    Graphic::Get().GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mesh.VertexBuffer));
    Graphic::Get().GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexStageBuff));

    mesh.VertexBufferView.BufferLocation = mesh.VertexBuffer->GetGPUVirtualAddress();
    mesh.VertexBufferView.SizeInBytes = sizeof(vertices);
    mesh.VertexBufferView.StrideInBytes = sizeof(DefaultVertex);

    // Index buffer
    Graphic::Get().GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mesh.IndexBuffer));
    Graphic::Get().GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexStageBuff));

    mesh.IndexBufferView.BufferLocation = mesh.IndexBuffer->GetGPUVirtualAddress();
    mesh.IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mesh.IndexBufferView.SizeInBytes = sizeof(indices);

    // Copy
    D3D12_SUBRESOURCE_DATA vertexData{};
    vertexData.pData = vertices;
    vertexData.RowPitch = sizeof(vertices);
    vertexData.SlicePitch = vertexData.RowPitch;
    UpdateSubresources<1>(copyCmdList.Get(), mesh.VertexBuffer, vertexStageBuff, 0, 0, 1, &vertexData);

    D3D12_SUBRESOURCE_DATA indexData{};
    indexData.pData = indices;
    indexData.RowPitch = sizeof(indices);
    indexData.SlicePitch = indexData.RowPitch;
    UpdateSubresources<1>(copyCmdList.Get(), mesh.IndexBuffer, indexStageBuff, 0, 0, 1, &indexData);

    // Post copy
    std::array<CD3DX12_RESOURCE_BARRIER, 2> barriers;
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(mesh.VertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(mesh.IndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    postCopyCmdList->ResourceBarrier(2, barriers.data());

    // Save stage buffers for later deletion
    mStageBuffers.insert(mStageBuffers.end(), { vertexStageBuff, indexStageBuff });
}
