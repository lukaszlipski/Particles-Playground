#pragma once
#include "Utilities/objectpool.h"

enum class ShaderType
{
    Vertex = 0,
    Pixel,
    Compute
};

class Shader : public IObject<Shader>
{
public:
    Shader(std::wstring_view name, ShaderType type, IDxcBlob* blob)
        : mName(name)
        , mType(type)
        , mBlob(blob)
    { }

    ~Shader()
    {
        mBlob->Release();
    }

    inline IDxcBlob* GetBlob() const { return mBlob; }
    inline ShaderType GetType() const { return mType; }
    inline std::wstring_view GetName() const { return mName; }
private:
    std::wstring mName;
    ShaderType mType;
    IDxcBlob* mBlob = nullptr;
};

using ShaderHandle = ObjectHandle<Shader>;
