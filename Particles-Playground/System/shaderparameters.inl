#include "commandlist.h"
#include "texture.h"
#include "gpubuffer.h"
#include "graphic.h"
#include "gpudescriptorheap.h"
#include "shaderparameterslayout.h"

template<typename T>
ShaderParameters& ShaderParameters::SetConstant(uint32_t idx, const T& data)
{
    const uint32_t size = std::max(static_cast<uint32_t>(sizeof(T) / 4), 1U);
    RootConstant constant;
    constant.Data.resize(size);

    memcpy(constant.Data.data(), &data, sizeof(T));

    mParams[idx] = std::move(constant);

    return *this;
}
