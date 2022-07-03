#include "System/dependencygraph.h"

DependencyGraph DependencyGraphBuilder::Build(const std::vector<uint32_t>& startPoints)
{
    mResourcesReady.clear();
    mAlreadyProcessed = std::vector<bool>(mSetupContexts->size());

    DependencyGraph dependencyGraph;

    std::vector<uint32_t> toProcessThisDepth;
    std::vector<uint32_t> toProcessNextDepth;

    PrepareExternalResources(*mExternalGPUBuffers, *mExternalTextures2D);
    toProcessThisDepth = startPoints; // Copy start points

    while (!toProcessThisDepth.empty())
    {
        std::vector<uint32_t>& currentDepth = dependencyGraph.emplace_back(std::vector<uint32_t>{});

        std::vector<uint32_t> nodesWithAliases;
        std::vector<uint32_t> nodesWithoutAliases;
        std::set<ResourceID> resourcesUsed;

        PartitionNodes(toProcessThisDepth, nodesWithoutAliases, nodesWithAliases, toProcessNextDepth);
        ProcessNodesWithoutAliases(nodesWithoutAliases, currentDepth, resourcesUsed, toProcessNextDepth);
        ProcessNodesWithAliases(nodesWithAliases, currentDepth, resourcesUsed, toProcessNextDepth);

        UpdateReadyResources(currentDepth);

        Assert(!currentDepth.empty()); // No nodes added this depth!
        toProcessThisDepth = std::move(toProcessNextDepth);
        toProcessNextDepth.clear();
    }

    return dependencyGraph;
}

void DependencyGraphBuilder::PrepareExternalResources(const std::map<ResourceID, GPUBuffer*>& externalGPUBuffers, const std::map<ResourceID, Texture2D*>& externalTextures2D)
{
    for (const std::pair<ResourceID, GPUBuffer*>& buffer : externalGPUBuffers)
    {
        mResourcesReady[buffer.first] = false;
    }

    for (const std::pair<ResourceID, Texture2D*>& texture : externalTextures2D)
    {
        mResourcesReady[texture.first] = false;
    }
}

bool DependencyGraphBuilder::AddToThisDepth(uint32_t nodeIndex, const RGSetupContext& setupContext, std::vector<uint32_t>& currentDepth, std::set<ResourceID>& resourcesUsed, std::vector<uint32_t>& toProcessNextDepth)
{
    if (mAlreadyProcessed[nodeIndex]) { return false; }
    mAlreadyProcessed[nodeIndex] = true;

    currentDepth.push_back(nodeIndex);
    resourcesUsed.insert(setupContext.GetInputs().cbegin(), setupContext.GetInputs().cend());

    const std::vector<uint32_t>& adjacentNodes = (*mAdjacencyList)[nodeIndex];
    for (uint32_t i = 0; i < adjacentNodes.size(); ++i)
    {
        const uint32_t adjacentIndex = adjacentNodes[i];

        if (!mAlreadyProcessed[adjacentIndex])
        {
            toProcessNextDepth.push_back(adjacentIndex);
        }
    }
    return true;
}

void DependencyGraphBuilder::PartitionNodes(const std::vector<uint32_t>& toProcessThisDepth, std::vector<uint32_t>& nodesWithoutAliases, std::vector<uint32_t>& nodesWithAliases, std::vector<uint32_t>& toProcessNextDepth)
{
    for (uint32_t nodeIndex : toProcessThisDepth)
    {
        if (mAlreadyProcessed[nodeIndex])
            continue;

        const RGSetupContext& setupContext = (*mSetupContexts)[nodeIndex];

        const std::vector<ResourceID>& inputs = setupContext.GetInputs();
        bool canBePlacedThisDepth = std::all_of(inputs.begin(), inputs.end(), [&resourcesReady = this->mResourcesReady](ResourceID id) {
            auto it = resourcesReady.find(id);
            if (it != resourcesReady.end())
            {
                Assert(it->second == false); // The resource has been aliased!
                return true;
            }
            return false;
            });

        if (canBePlacedThisDepth)
        {
            if (!setupContext.GetAliases().empty())
            {
                // Nodes with aliases should be processed separately as these nodes could alias resources that are used by current depth
                nodesWithAliases.push_back(nodeIndex);
                continue;
            }
            nodesWithoutAliases.push_back(nodeIndex);
        }
        else
        {
            toProcessNextDepth.push_back(nodeIndex);
        }
    }
}

void DependencyGraphBuilder::ProcessNodesWithoutAliases(const std::vector<uint32_t>& nodesWithoutAliases, std::vector<uint32_t>& currentDepth, std::set<ResourceID>& resourcesUsed, std::vector<uint32_t>& toProcessNextDepth)
{
    for (uint32_t nodeIndex : nodesWithoutAliases)
    {
        const RGSetupContext& setupContext = (*mSetupContexts)[nodeIndex];

        AddToThisDepth(nodeIndex, setupContext, currentDepth, resourcesUsed, toProcessNextDepth);
    }
}

void DependencyGraphBuilder::ProcessNodesWithAliases(const std::vector<uint32_t>& nodesWithAliases, std::vector<uint32_t>& currentDepth, std::set<ResourceID>& resourcesUsed, std::vector<uint32_t>& toProcessNextDepth)
{
    for (uint32_t nodeIndex : nodesWithAliases)
    {
        const RGSetupContext& setupContext = (*mSetupContexts)[nodeIndex];
        const std::vector<std::pair<ResourceID, ResourceID>>& aliases = setupContext.GetAliases();

        const bool safeToAddThisDepth = std::none_of(aliases.begin(), aliases.end(), [&resourcesUsed](const std::pair<ResourceID, ResourceID>& alias) {
            return resourcesUsed.find(alias.first) != resourcesUsed.end();
            });

        if (safeToAddThisDepth)
        {
            AddToThisDepth(nodeIndex, setupContext, currentDepth, resourcesUsed, toProcessNextDepth);
        }
        else
        {
            toProcessNextDepth.push_back(nodeIndex);
        }
    }
}

void DependencyGraphBuilder::UpdateReadyResources(const std::vector<uint32_t>& currentDepth)
{
    for (uint32_t nodeIndex : currentDepth)
    {
        const RGSetupContext& setupContext = (*mSetupContexts)[nodeIndex];
        const std::vector<ResourceID>& outputs = setupContext.GetOutputs();
        for (ResourceID id : outputs)
        {
            mResourcesReady[id] = false;
        }

        const std::vector<std::pair<ResourceID, ResourceID>>& aliases = setupContext.GetAliases();

        for (const std::pair<ResourceID, ResourceID>& alias : aliases)
        {
            // #NOTE: Mark a resource as being aliased, this is going to help with checking
            //        if a node tries to access an already aliased resource which should never happen
            mResourcesReady[alias.first] = true;
            Assert(mResourcesReady.find(alias.second) != mResourcesReady.end());
            Assert(mResourcesReady[alias.second] == false);
        }
    }
}
