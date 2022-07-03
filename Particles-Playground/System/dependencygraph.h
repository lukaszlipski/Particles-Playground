#pragma once
#include "System/rendergraphcommon.h"

using DependencyGraph = std::vector<std::vector<uint32_t>>;

class DependencyGraphBuilder
{
public:
    explicit DependencyGraphBuilder(const std::vector<RGSetupContext>& setupContexts, const std::vector<std::vector<uint32_t>>& adjacencyList, const std::map<ResourceID, GPUBuffer*>& externalGPUBuffers, const std::map<ResourceID, Texture2D*>& externalTextures2D)
        : mSetupContexts(&setupContexts)
        , mAdjacencyList(&adjacencyList)
        , mExternalGPUBuffers(&externalGPUBuffers)
        , mExternalTextures2D(&externalTextures2D)
    { }

    ~DependencyGraphBuilder() = default;

    DependencyGraphBuilder(const DependencyGraphBuilder&) = delete;
    DependencyGraphBuilder(DependencyGraphBuilder&&) = delete;

    DependencyGraphBuilder& operator=(const DependencyGraphBuilder&) = delete;
    DependencyGraphBuilder& operator=(DependencyGraphBuilder&&) = delete;

    DependencyGraph Build(const std::vector<uint32_t>& startPoints);

private:
    void PrepareExternalResources(const std::map<ResourceID, GPUBuffer*>& externalGPUBuffers, const std::map<ResourceID, Texture2D*>& externalTextures2D);
    bool AddToThisDepth(uint32_t nodeIndex, const RGSetupContext& setupContext, std::vector<uint32_t>& currentDepth, std::set<ResourceID>& resourcesUsed, std::vector<uint32_t>& toProcessNextDepth);;
    void PartitionNodes(const std::vector<uint32_t>& toProcessThisDepth, std::vector<uint32_t>& nodesWithoutAliases, std::vector<uint32_t>& nodesWithAliases, std::vector<uint32_t>& toProcessNextDepth);
    void ProcessNodesWithoutAliases(const std::vector<uint32_t>& nodesWithoutAliases, std::vector<uint32_t>& currentDepth, std::set<ResourceID>& resourcesUsed, std::vector<uint32_t>& toProcessNextDepth);
    void ProcessNodesWithAliases(const std::vector<uint32_t>& nodesWithAliases, std::vector<uint32_t>& currentDepth, std::set<ResourceID>& resourcesUsed, std::vector<uint32_t>& toProcessNextDepth);
    void UpdateReadyResources(const std::vector<uint32_t>& currentDepth);

    const std::vector<RGSetupContext>* mSetupContexts = nullptr;
    const std::vector<std::vector<uint32_t>>* mAdjacencyList = nullptr;
    const std::map<ResourceID, GPUBuffer*>* mExternalGPUBuffers = nullptr;
    const std::map<ResourceID, Texture2D*>* mExternalTextures2D = nullptr;
    std::map<ResourceID, bool> mResourcesReady;
    std::vector<bool> mAlreadyProcessed;

};
