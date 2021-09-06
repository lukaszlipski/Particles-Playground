
struct ResetConstants
{
    uint Offset;
    uint IndicesCount;
};

ConstantBuffer<ResetConstants> Constants : register(b0, space0);
RWStructuredBuffer<uint> FreeIndices : register(u0, space0);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x < Constants.IndicesCount)
    {
        FreeIndices[Constants.Offset + id.x] = id.x;
    }
}
