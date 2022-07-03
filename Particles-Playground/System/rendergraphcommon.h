#pragma once
#include "System/gpubuffer.h"
#include "System/texture.h"

// #TODO: make a proper, compile time resource id system
using ResourceID = std::string;
#define RESOURCEID(x) std::string{x}

class RGSetupContext;
class RGExecuteContext;
class GPUParticleSystem;
class Camera;

class IRenderNodeBase
{
public:
    virtual void Setup(RGSetupContext& context) = 0;
    virtual void Execute(const RGExecuteContext& context) = 0;
};

struct RGNewGPUBuffer
{
    uint32_t mElemSize = 0;
    uint32_t mNumElems = 0;
    BufferUsage mUsage = BufferUsage::All;
};

struct RGNewTexture2D
{
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    TextureFormat mFormat = TextureFormat::R8G8B8A8;
    TextureUsage mUsage = TextureUsage::All;
};

struct RGGPUBuffer
{
    ResourceID mID = 0;
    BufferUsage mUsage = BufferUsage::All;
};

struct RGTexture2D
{
    ResourceID mID = 0;
    TextureUsage mUsage = TextureUsage::All;
};

class RGSetupContext
{
public:
    RGSetupContext& InputGPUBuffer(ResourceID resId, BufferUsage usage)
    {
        Assert(!static_cast<BufferUsageType>(usage & BufferUsage::UnorderedAccess) || !static_cast<BufferUsageType>(usage & BufferUsage::CopyDst));
        mGPUBuffers.push_back(RGGPUBuffer{ resId, usage });
        return SetInput(resId);
    }

    RGSetupContext& InputTexture2D(ResourceID resId, TextureUsage usage)
    {
        Assert(!static_cast<TextureUsageType>(usage & TextureUsage::RenderTarget) || !static_cast<TextureUsageType>(usage & TextureUsage::DepthWrite) || !static_cast<TextureUsageType>(usage & TextureUsage::CopyDst));
        mTextures2D.push_back(RGTexture2D{ resId, usage });
        return SetInput(resId);
    }

    RGNewGPUBuffer& OutputGPUBuffer(ResourceID resId, BufferUsage usage)
    {
        Assert(static_cast<BufferUsageType>(usage & BufferUsage::UnorderedAccess) || static_cast<BufferUsageType>(usage & BufferUsage::CopyDst));
        mGPUBuffers.push_back(RGGPUBuffer{ resId, usage });
        SetOutput(resId);

        std::pair<ResourceID, RGNewGPUBuffer>& newBuffer = mNewBuffers.emplace_back();
        newBuffer.first = resId;
        return newBuffer.second;
    }

    RGNewTexture2D& OutputTexture2D(ResourceID resId, TextureUsage usage)
    {
        Assert(static_cast<BufferUsageType>(usage & TextureUsage::RenderTarget) || static_cast<BufferUsageType>(usage & TextureUsage::DepthWrite) || static_cast<TextureUsageType>(usage & TextureUsage::CopyDst));
        mTextures2D.push_back(RGTexture2D{ resId, usage });
        SetOutput(resId);

        std::pair<ResourceID, RGNewTexture2D>& newTexture = mNewTextures2D.emplace_back();
        newTexture.first = resId;
        return newTexture.second;
    }

    RGSetupContext& InputOutputGPUBuffer(ResourceID inResId, ResourceID outResId, BufferUsage usage)
    {
        Assert(static_cast<BufferUsageType>(usage & BufferUsage::UnorderedAccess) || static_cast<BufferUsageType>(usage & BufferUsage::CopyDst));
        mGPUBuffers.push_back(RGGPUBuffer{ inResId, usage });
        return SetAlias(inResId, outResId);
    }

    RGSetupContext& InputOutputTexture2D(ResourceID inResId, ResourceID outResId, TextureUsage usage)
    {
        Assert(static_cast<TextureUsageType>(usage & TextureUsage::RenderTarget) || static_cast<TextureUsageType>(usage & TextureUsage::DepthWrite) || static_cast<TextureUsageType>(usage & TextureUsage::CopyDst));
        mTextures2D.push_back(RGTexture2D{ inResId, usage });
        return SetAlias(inResId, outResId);
    }

    inline const std::vector<ResourceID>& GetInputs() const { return mInputs; }
    inline const std::vector<ResourceID>& GetOutputs() const { return mOutputs; }
    inline const std::vector<std::pair<ResourceID, ResourceID>>& GetAliases() const { return mAliases; }
    inline const std::vector<RGGPUBuffer>& GetGPUBuffers() const { return mGPUBuffers; }
    inline const std::vector<RGTexture2D>& GetTextures2D() const { return mTextures2D; }
    inline const std::vector<std::pair<ResourceID, RGNewGPUBuffer>>& GetNewBuffers() const { return mNewBuffers; }
    inline const std::vector<std::pair<ResourceID, RGNewTexture2D>>& GetNewTextures2D() const { return mNewTextures2D; }

private:
    RGSetupContext& SetInput(ResourceID resId)
    {
        Assert(std::find(mInputs.cbegin(), mInputs.cend(), resId) == mInputs.cend());
        mInputs.push_back(resId);
        return *this;
    }

    RGSetupContext& SetOutput(ResourceID resId)
    {
        Assert(std::find(mOutputs.cbegin(), mOutputs.cend(), resId) == mOutputs.end());
        mOutputs.push_back(resId);
        return *this;
    }

    RGSetupContext& SetAlias(ResourceID inResId, ResourceID outResId)
    {
        SetInput(inResId);
        SetOutput(outResId);

        mAliases.push_back({ inResId, outResId });
        return *this;
    }

    std::vector<ResourceID> mInputs;
    std::vector<ResourceID> mOutputs;
    std::vector<std::pair<ResourceID, ResourceID>> mAliases;

    std::vector<RGGPUBuffer> mGPUBuffers;
    std::vector<RGTexture2D> mTextures2D;

    std::vector<std::pair<ResourceID, RGNewGPUBuffer>> mNewBuffers;
    std::vector<std::pair<ResourceID, RGNewTexture2D>> mNewTextures2D;
};

struct SceneData
{
    GPUParticleSystem* mGPUParticleSystem = nullptr;
    Camera* mCamera = nullptr;
};

class RGExecuteContext
{
public:
    RGExecuteContext(std::map<ResourceID, GPUBuffer*>&& availableGPUBuffers, std::map<ResourceID, Texture2D*>&& availableTextures2D, SceneData& sceneData, CommandList& cmdList)
        : mAvailableGPUBuffers(std::move(availableGPUBuffers))
        , mAvailableTextures2D(std::move(availableTextures2D))
        , mSceneData(sceneData)
        , mCommandList(cmdList)
    { }

    ~RGExecuteContext() = default;

    GPUBuffer* GetGPUBuffer(ResourceID id) const
    {
        auto it = mAvailableGPUBuffers.find(id);
        Assert(it != mAvailableGPUBuffers.end()); // Resource is not available
        GPUBuffer* resource = it->second;
        return resource;
    }

    Texture2D* GetTexture2D(ResourceID id) const
    {
        auto it = mAvailableTextures2D.find(id);
        Assert(it != mAvailableTextures2D.end()); // Resource is not available
        Texture2D* resource = it->second;
        return resource;
    }

    inline CommandList& GetCommandList() const { return mCommandList; }
    inline SceneData& GetSceneData() const { return mSceneData; }

private:
    std::map<ResourceID, GPUBuffer*> mAvailableGPUBuffers;
    std::map<ResourceID, Texture2D*> mAvailableTextures2D;
    std::reference_wrapper<CommandList> mCommandList;
    std::reference_wrapper<SceneData> mSceneData;
};
