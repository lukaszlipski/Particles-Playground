#include "System/rendergraph.h"
#include "System/commandlist.h"
#include "System/graphic.h"
#include "System/gpudescriptorheap.h"

void RenderGraph::Setup()
{
    Assert(mEndPointsIndices.size()); // The graph has to contain at least one end point

    mSetupContexts.clear();
    mDependencyGraph.clear();
    mResourcesLifetimes.clear();

    mSetupContexts = GatherSetupContexts();

    std::vector<uint32_t> startPoints;
    std::vector<std::vector<uint32_t>> adjacencyList = BuildAdjacencyList(mSetupContexts, startPoints);
    Assert(!startPoints.empty());

    // Check if graph contains any cycles
    std::vector<bool> alreadyProcessed;
    std::vector<bool> currentPath(mNodes.size());
    for (uint32_t startPoint : startPoints)
    {
        alreadyProcessed.clear();
        alreadyProcessed.resize(mNodes.size(), false);
        DetectCycleDFS(startPoint, mSetupContexts, adjacencyList, alreadyProcessed, currentPath);
    }

    DependencyGraphBuilder builder(mSetupContexts, adjacencyList, mExternalGPUBuffers, mExternalTextures2D);
    mDependencyGraph = builder.Build(startPoints);

    mResourcesLifetimes = CalculateResourcesLifeTimes(mSetupContexts, mDependencyGraph);
}

void RenderGraph::Execute(TransientResourceAllocator& allocator, SceneData& sceneData)
{
    if (mDependencyGraph.empty()) { return; }

    CommandList commandList(QueueType::Direct);

    std::array<ID3D12DescriptorHeap*, 1> descHeaps = { Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap() };
    commandList->SetDescriptorHeaps(static_cast<uint32_t>(descHeaps.size()), descHeaps.data());

    std::map<ResourceID, TransientResourceHandle> resources;
    std::map<ResourceID, ResourceID> availableAliases;

    // Make a copy of resource lifetimes to allow a graph's setup reuse
    std::map<ResourceID, uint32_t> lifeTimes = mResourcesLifetimes;

    for (uint32_t depthLevel = 0; depthLevel < mDependencyGraph.size(); ++depthLevel)
    {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        AllocateTransientResources(mDependencyGraph[depthLevel], resources, allocator, barriers);
        PrepapreResourceBarriers(mDependencyGraph[depthLevel], availableAliases, resources, allocator, barriers);

        // Issue aliasing and resource barriers for current depth level
        if (barriers.size())
        {
            commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        }

        ClearResourcesForCurrentDepth(commandList, mDependencyGraph[depthLevel], availableAliases, resources, allocator);
        ExecuteNodesForCurrentDepth(commandList, mDependencyGraph[depthLevel], availableAliases, resources, allocator, sceneData);
        ReleaseResourcesForCurrentDepth(mDependencyGraph[depthLevel], availableAliases, lifeTimes, resources, allocator);

    }

    commandList.Submit();
}

std::vector<RGSetupContext> RenderGraph::GatherSetupContexts() const
{
    if (mNodes.empty()) { return {}; };

    std::vector<RGSetupContext> setupContexts(mNodes.size());
    std::set<ResourceID> uniqueResources;

    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        IRenderNodeBase* const node = mNodes[i].get();
        RGSetupContext& context = setupContexts[i];
        node->Setup(context);

        const std::vector<ResourceID>& inputs = context.GetInputs();
        const std::vector<ResourceID>& outputs = context.GetOutputs();

        // Self cycle detected!
        Assert(std::find_first_of(inputs.cbegin(), inputs.cend(), outputs.cbegin(), outputs.cend()) == inputs.cend());

        // The same output defined twice in two different nodes!
        Assert(std::find_first_of(uniqueResources.cbegin(), uniqueResources.cend(), outputs.cbegin(), outputs.cend()) == uniqueResources.cend());
        uniqueResources.insert(outputs.begin(), outputs.end());
    }

    return setupContexts;
}

std::vector<std::vector<uint32_t>> RenderGraph::BuildAdjacencyList(const std::vector<RGSetupContext>& setupContexts, std::vector<uint32_t>& startPoints) const
{
    startPoints.clear();

    std::vector<std::vector<uint32_t>> adjacencyList(mNodes.size());
    std::list<uint32_t> indicesToProcess(mEndPointsIndices.begin(), mEndPointsIndices.end());
    std::vector<bool> alreadyProcessed(mNodes.size());

    while (!indicesToProcess.empty())
    {
        const uint32_t index = indicesToProcess.back();
        indicesToProcess.pop_back();
        alreadyProcessed[index] = true;

        const RGSetupContext& context = setupContexts[index];

        // Check if starting point
        bool startingPoint = std::all_of(context.GetInputs().begin(), context.GetInputs().end(), [&](ResourceID id) {
            return mExternalGPUBuffers.find(id) != mExternalGPUBuffers.end() || mExternalTextures2D.find(id) != mExternalTextures2D.end();
        });
        if (startingPoint)
        {
            startPoints.push_back(index);
        }

        for (uint32_t nodeIndex = 0; nodeIndex < mNodes.size(); ++nodeIndex)
        {
            if (index == nodeIndex)
                continue;

            const RGSetupContext& nodeContext = setupContexts[nodeIndex];

            for (ResourceID outputId : nodeContext.GetOutputs())
            {
                auto itr = std::find_if(context.GetInputs().cbegin(), context.GetInputs().cend(), [outputId](ResourceID inputId) {
                    return inputId == outputId;
                });

                if (itr != context.GetInputs().cend())
                {
                    if (!alreadyProcessed[nodeIndex])
                    {
                        // Only process this node if it is on the path to one of the end points and hasn't been marked as to be processed yet
                        indicesToProcess.push_front(nodeIndex);
                    }

                    adjacencyList[nodeIndex].push_back(index);
                    break;
                }
            }

        }
    }

    return adjacencyList;
}

void RenderGraph::DetectCycleDFS(uint32_t nodeIndex, const std::vector<RGSetupContext>& setupContexts, const std::vector<std::vector<uint32_t>>& adjacencyList, std::vector<bool>& alreadyProcessed, std::vector<bool>& currentPath) const
{
    if (alreadyProcessed[nodeIndex])
    {
        return;
    }

    alreadyProcessed[nodeIndex] = true;
    currentPath[nodeIndex] = true;

    for (uint32_t adjacentIndex : adjacencyList[nodeIndex])
    {
        Assert(!currentPath[adjacentIndex]); // Cycle detected!!!
        DetectCycleDFS(adjacentIndex, setupContexts, adjacencyList, alreadyProcessed, currentPath);
    }

    currentPath[nodeIndex] = false;
}

std::map<ResourceID, uint32_t> RenderGraph::CalculateResourcesLifeTimes(const std::vector<RGSetupContext>& setupContexts, const std::vector<std::vector<uint32_t>>& dependencyGraph) const
{
    std::map<ResourceID, uint32_t> resourcesLifetimes;
    std::map<ResourceID, ResourceID> availableAliases;

    for (uint32_t depthLevel = 0; depthLevel < dependencyGraph.size(); ++depthLevel)
    {
        const std::vector<uint32_t>& currentDepth = dependencyGraph[depthLevel];

        for (uint32_t nodeIndex : currentDepth)
        {
            const std::vector<ResourceID>& inputs = setupContexts[nodeIndex].GetInputs();
            const std::vector<ResourceID>& outputs = setupContexts[nodeIndex].GetOutputs();
            const std::vector<std::pair<ResourceID, ResourceID>>& aliases = setupContexts[nodeIndex].GetAliases();

            for (std::pair<ResourceID, ResourceID> alias : aliases)
            {
                availableAliases[alias.second] = alias.first;
            }

            for (ResourceID id : inputs)
            {
                ++resourcesLifetimes[GetRealResourceID(id, availableAliases)];
            }

            for (ResourceID id : outputs)
            {
                ++resourcesLifetimes[GetRealResourceID(id, availableAliases)];
            }
        }
    }

    return resourcesLifetimes;
}

void RenderGraph::AllocateTransientResources(const std::vector<uint32_t>& nodes, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator, std::vector<D3D12_RESOURCE_BARRIER>& barriers)
{
    for (uint32_t nodeIndex : nodes)
    {
        const RGSetupContext& setupContext = mSetupContexts[nodeIndex];

        for (const std::pair<ResourceID, RGNewGPUBuffer>& newBuffer : setupContext.GetNewBuffers())
        {
            const ResourceID id = newBuffer.first;
            const RGNewGPUBuffer& bufferInfo = newBuffer.second;
            resources[id] = allocator.AllocateGPUBuffer(bufferInfo.mElemSize, bufferInfo.mNumElems, bufferInfo.mUsage);
        }

        for (const std::pair<ResourceID, RGNewTexture2D>& newTexture : setupContext.GetNewTextures2D())
        {
            const ResourceID id = newTexture.first;
            const RGNewTexture2D& textureInfo = newTexture.second;
            resources[id] = allocator.AllocateTexture2D(textureInfo.mWidth, textureInfo.mHeight, textureInfo.mFormat, textureInfo.mUsage);
        }
    }

    allocator.Step(barriers);
}

void RenderGraph::PrepapreResourceBarriers(const std::vector<uint32_t>& nodes, std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator, std::vector<D3D12_RESOURCE_BARRIER>& barriers)
{
    for (uint32_t nodeIndex : nodes)
    {
        const RGSetupContext& setupContext = mSetupContexts[nodeIndex];

        for (const std::pair<ResourceID, ResourceID>& alias : setupContext.GetAliases())
        {
            availableAliases[alias.second] = alias.first;
        }

        for (const RGGPUBuffer& gpuBuffer : setupContext.GetGPUBuffers())
        {
            const ResourceID id = GetRealResourceID(gpuBuffer.mID, availableAliases);

            GPUBuffer* buffer = nullptr;
            if (resources.find(id) != resources.end())
            {
                buffer = allocator.GetResource<GPUBuffer>(resources[id]);
            }
            else
            {
                auto it = mExternalGPUBuffers.find(id);
                Assert(it != mExternalGPUBuffers.end());
                buffer = it->second;
            }

            Assert(buffer);
            if (buffer->HasBufferUsage(BufferUsage::UnorderedAccess) && (gpuBuffer.mUsage == BufferUsage::UnorderedAccess))
            {
                D3D12_RESOURCE_BARRIER& barrier = barriers.emplace_back();
                barrier = CD3DX12_RESOURCE_BARRIER::UAV(buffer->GetResource());
            }
            buffer->SetCurrentUsage(gpuBuffer.mUsage, barriers);
        }

        for (const RGTexture2D& texture : setupContext.GetTextures2D())
        {
            const ResourceID id = GetRealResourceID(texture.mID, availableAliases);
            Texture2D* tex = nullptr;
            if (resources.find(id) != resources.end())
            {
                tex = allocator.GetResource<Texture2D>(resources[id]);
            }
            else
            {
                auto it = mExternalTextures2D.find(id);
                Assert(it != mExternalTextures2D.end());
                tex = it->second;
            }

            Assert(tex);
            tex->SetCurrentUsage(texture.mUsage, false, barriers);
        }
    }
}

void RenderGraph::ClearResourcesForCurrentDepth(CommandList& cmdList, const std::vector<uint32_t>& nodes, std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator)
{
    for (uint32_t nodeIndex : nodes)
    {
        const RGSetupContext& setupContext = mSetupContexts[nodeIndex];

        for (const std::pair<ResourceID, RGNewTexture2D>& newTexture : setupContext.GetNewTextures2D())
        {
            const ResourceID id = newTexture.first;
            ResourceID realId = GetRealResourceID(id, availableAliases);

            Assert(resources.find(realId) != resources.end());
            Texture2D* texture = allocator.GetResource<Texture2D>(resources[realId]);

            if (texture->HasTextureUsage(TextureUsage::RenderTarget))
            {
                std::optional<D3D12_CLEAR_VALUE> clearValue = texture->GetClearValue();
                float* clearColor = clearValue.has_value() ? clearValue.value().Color : nullptr;
                Assert(clearColor);
                cmdList->ClearRenderTargetView(texture->GetRTV(), clearColor, 0, nullptr);
            }
            else if (texture->HasTextureUsage(TextureUsage::DepthWrite))
            {
                std::optional<D3D12_CLEAR_VALUE> clearValue = texture->GetClearValue();
                Assert(clearValue.has_value());
                cmdList->ClearDepthStencilView(texture->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, clearValue.value().DepthStencil.Depth, 0, 0, nullptr);
            }
        }
    }
}

void RenderGraph::ExecuteNodesForCurrentDepth(CommandList& cmdList, const std::vector<uint32_t>& nodes, const std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator, SceneData& sceneData)
{
    for (uint32_t nodeIndex : nodes)
    {
        const IRenderNodeBase* node = mNodes[nodeIndex].get();
        Assert(node);
        const char* className = typeid(*node).name();
        PIXScopedEvent(cmdList.Get(), 0, className);

        const RGSetupContext& setupContext = mSetupContexts[nodeIndex];

        // Prepare resources which are needed by current node
        std::map<ResourceID, GPUBuffer*> availableGPUBuffers;
        std::map<ResourceID, Texture2D*> availableTextures2D;

        for (const RGGPUBuffer& gpuBuffer : setupContext.GetGPUBuffers())
        {
            const ResourceID id = gpuBuffer.mID;
            const ResourceID realId = GetRealResourceID(id, availableAliases);

            GPUBuffer* buffer = nullptr;
            if (resources.find(realId) != resources.end())
            {
                buffer = allocator.GetResource<GPUBuffer>(resources[realId]);
            }
            else
            {
                auto it = mExternalGPUBuffers.find(realId);
                Assert(it != mExternalGPUBuffers.end());
                buffer = it->second;
            }

            Assert(buffer);
            availableGPUBuffers[id] = buffer;
        }

        for (const RGTexture2D& texture : setupContext.GetTextures2D())
        {
            const ResourceID id = texture.mID;
            const ResourceID realId = GetRealResourceID(id, availableAliases);

            Texture2D* tex = nullptr;
            if (resources.find(realId) != resources.end())
            {
                tex = allocator.GetResource<Texture2D>(resources[realId]);
            }
            else
            {
                auto it = mExternalTextures2D.find(realId);
                Assert(it != mExternalTextures2D.end());
                tex = it->second;
            }

            Assert(tex);
            availableTextures2D[id] = tex;
        }

        RGExecuteContext executeContext(std::move(availableGPUBuffers), std::move(availableTextures2D), sceneData, cmdList);
        mNodes[nodeIndex]->Execute(executeContext);
    }
}

void RenderGraph::ReleaseResourcesForCurrentDepth(const std::vector<uint32_t>& nodes, const std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, uint32_t>& lifeTimes, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator)
{
    for (uint32_t nodeIndex : nodes)
    {
        const RGSetupContext& setupContext = mSetupContexts[nodeIndex];

        for (ResourceID id : setupContext.GetInputs())
        {
            ResourceID realId = GetRealResourceID(id, availableAliases);

            if (resources.find(realId) != resources.end() && --lifeTimes[realId] == 0)
            {
                allocator.FreeResource(resources[realId]);
            }
        }

        for (ResourceID id : setupContext.GetOutputs())
        {
            ResourceID realId = GetRealResourceID(id, availableAliases);

            if (resources.find(realId) != resources.end() && --lifeTimes[realId] == 0)
            {
                allocator.FreeResource(resources[realId]);
            }
        }
    }
}

ResourceID RenderGraph::GetRealResourceID(ResourceID id, const std::map<ResourceID, ResourceID>& availableAliases) const
{
    ResourceID realId = id;
    auto it = availableAliases.find(realId);
    while (it != availableAliases.end())
    {
        realId = it->second;
        it = availableAliases.find(realId);
    }
    return realId;
}
