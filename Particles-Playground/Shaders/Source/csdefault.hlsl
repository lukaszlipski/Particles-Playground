
RWStructuredBuffer<int> Test : register(u0, space0);

[numthreads(64, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
    Test[id.x] = id.x;
}
