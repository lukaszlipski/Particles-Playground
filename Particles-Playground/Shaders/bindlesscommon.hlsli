#ifndef __BINDLESS_COMMON_HLSL__
#define __BINDLESS_COMMON_HLSL__

typedef uint32_t BindlessDescriptorHandle;
static const uint32_t BindlessDescriptorRegisterSpace = 1;

#ifdef __hlsl_dx_compiler

#if __SHADER_TARGET_MINOR >= 6 && ENABLE_RESOURCE_DESCRIPTOR_HEAP == 1
    #define GET_TYPED_RESOURCE_HEAP(resource, type) ResourceDescriptorHeap
    #define DEFINE_BINDLESS_UAV_TYPED_RESOURCE_HEAP(resource, type)
    #define DEFINE_BINDLESS_SRV_TYPED_RESOURCE_HEAP(resource, type)
    #define DEFINE_BINDLESS_CBV_TYPED_RESOURCE_HEAP(resource, type)
#else
    #define GET_TYPED_RESOURCE_HEAP(resource, type) g_##resource##type
    #define DEFINE_BINDLESS_UAV_TYPED_RESOURCE_HEAP(resource, type) resource<type> g_##resource##type[] : register(u0, space1)
    #define DEFINE_BINDLESS_SRV_TYPED_RESOURCE_HEAP(resource, type) resource<type> g_##resource##type[] : register(t0, space1)
    #define DEFINE_BINDLESS_CBV_TYPED_RESOURCE_HEAP(resource, type) resource<type> g_##resource##type[] : register(b0, space1)
#endif

#define GET_TYPED_RESOURCE_UNIFORM(resource, type, handle) GET_TYPED_RESOURCE_HEAP(resource, type)[handle]
#define GET_TYPED_RESOURCE(resource, type, handle) GET_TYPED_RESOURCE_HEAP(resource, type)[NonUniformResourceIndex(handle)];

#endif

#endif // __BINDLESS_COMMON_HLSL__
