#pragma once
#include "Utilities/memory.h"
#include "System/vertexformats.h"

class CommandList;
class ShaderParametersLayout;

template<typename T>
class PipelineState
{
public:
    uint32_t Hash() const
    {
        const uint32_t* start = reinterpret_cast<const uint32_t*>(&mState);
        const uint32_t* end = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(&mState) + AlignPow2(sizeof(mState), 4));

        return HashRange(start, end);
    }

    operator T() const
    {
        return mState;
    }

protected:
    alignas(32) T mState = {};

    static constexpr size_t padSize = std::max(AlignPow2(sizeof(mState), 4) - sizeof(mState), size_t(1));
    uint8_t mPad[padSize] = { 0 };

};

class GraphicPipelineState : public PipelineState<D3D12_GRAPHICS_PIPELINE_STATE_DESC>
{
public:
    GraphicPipelineState();

    template<typename T>
    GraphicPipelineState& SetVertexFormat()
    {
        const VertexFormatDescRef vertexFormatDef = GetVertexFormatDesc<T>();
        mState.InputLayout = { vertexFormatDef->data(), static_cast<uint32_t>(vertexFormatDef->size()) };

        return *this;
    }

    GraphicPipelineState& SetVS(std::wstring_view name);
    GraphicPipelineState& SetPS(std::wstring_view name);
    GraphicPipelineState& SetRTNum(uint32_t num);
    GraphicPipelineState& SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
    GraphicPipelineState& SetRTFormat(uint32_t idx, DXGI_FORMAT format);
    GraphicPipelineState& SetRTBlendState(uint32_t idx, const D3D12_RENDER_TARGET_BLEND_DESC& blend);
    GraphicPipelineState& SetIndependentBlend(bool enable);
    GraphicPipelineState& SetViewportProperties(uint32_t idx, const CD3DX12_VIEWPORT& properties);
    void Bind(CommandList& commandList, ShaderParametersLayout& layout);

private:
    std::array<CD3DX12_VIEWPORT, 8> mViewports;
    std::array<CD3DX12_RECT, 8> mScissorRects;

};

