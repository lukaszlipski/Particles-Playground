#pragma once
#include "System/rendergraphcommon.h"
#include "System/dependencygraph.h"
#include "Utilities/debug.h"
#include "System/transientresourceallocator.h"

class RenderGraph
{
public:
    RenderGraph() = default;
    ~RenderGraph() = default;

    RenderGraph(const RenderGraph&) = default;
    RenderGraph(RenderGraph&&) = default;

    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph& operator=(RenderGraph&&) = delete;

    template<typename NodeType, typename = std::enable_if_t<std::is_base_of_v<IRenderNodeBase, NodeType>>>
    RenderGraph& AddNode(bool endPoint = false)
    {
        if (endPoint)
        {
            // End points are the only required nodes that have to be processed by a graph. 
            // Based on end nodes' dependencies, other nodes can be executed.
            mEndPointsIndices.push_back(static_cast<uint32_t>(mNodes.size()));
        }

        mNodes.push_back(std::make_unique<NodeType>());
        return *this;
    }

    void Setup();
    void Execute(TransientResourceAllocator& allocator, SceneData& sceneData);

    inline void AddExternalGPUBuffer(ResourceID id, GPUBuffer* buffer) { mExternalGPUBuffers[id] = buffer; }
    inline void AddExternalTexture2D(ResourceID id, Texture2D* texture) { mExternalTextures2D[id] = texture; }

private:
    // Setup
    std::vector<RGSetupContext> GatherSetupContexts() const;
    std::vector<std::vector<uint32_t>> BuildAdjacencyList(const std::vector<RGSetupContext>& setupContexts, std::vector<uint32_t>& startPoints) const;
    void DetectCycleDFS(uint32_t nodeIndex, const std::vector<RGSetupContext>& setupContexts, const std::vector<std::vector<uint32_t>>& adjacencyList, std::vector<bool>& alreadyProcessed, std::vector<bool>& currentPath) const;
    std::map<ResourceID, uint32_t> CalculateResourcesLifeTimes(const std::vector<RGSetupContext>& setupContexts, const std::vector<std::vector<uint32_t>>& dependencyGraph) const;

    // Execute
    void AllocateTransientResources(const std::vector<uint32_t>& nodes, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator, std::vector<D3D12_RESOURCE_BARRIER>& barriers);
    void PrepapreResourceBarriers(const std::vector<uint32_t>& nodes, std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator, std::vector<D3D12_RESOURCE_BARRIER>& barriers);
    void ClearResourcesForCurrentDepth(CommandList& cmdList, const std::vector<uint32_t>& nodes, std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator);
    void ExecuteNodesForCurrentDepth(CommandList& cmdList, const std::vector<uint32_t>& nodes, const std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator, SceneData& sceneData);
    void ReleaseResourcesForCurrentDepth(const std::vector<uint32_t>& nodes, const std::map<ResourceID, ResourceID>& availableAliases, std::map<ResourceID, uint32_t>& lifeTimes, std::map<ResourceID, TransientResourceHandle>& resources, TransientResourceAllocator& allocator);

    ResourceID GetRealResourceID(ResourceID id, const std::map<ResourceID, ResourceID>& availableAliases) const;

    std::vector<std::unique_ptr<IRenderNodeBase>> mNodes;
    std::vector<uint32_t> mEndPointsIndices;
    std::vector<RGSetupContext> mSetupContexts;
    DependencyGraph mDependencyGraph;
    std::map<ResourceID, uint32_t> mResourcesLifetimes;

    std::map<ResourceID, GPUBuffer*> mExternalGPUBuffers;
    std::map<ResourceID, Texture2D*> mExternalTextures2D;
};
