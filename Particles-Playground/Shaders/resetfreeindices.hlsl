#include "bindlesscommon.hlsli"

DEFINE_BINDLESS_UAV_TYPED_RESOURCE_HEAP(RWStructuredBuffer, uint);

struct ResetConstants
{
    BindlessDescriptorHandle FreeIndicesHandle;
    uint Offset;
    uint IndicesCount;
};

ConstantBuffer<ResetConstants> Constants : register(b0, space0);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x < Constants.IndicesCount)
    {
        RWStructuredBuffer<uint> freeIndicesBuffer = GET_TYPED_RESOURCE_UNIFORM(RWStructuredBuffer, uint, Constants.FreeIndicesHandle);
        freeIndicesBuffer[Constants.Offset + id.x] = id.x;
    }
}
