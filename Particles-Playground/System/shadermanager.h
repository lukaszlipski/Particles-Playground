#pragma once

struct IDxcLibrary;
struct IDxcCompiler;
struct IDxcBlob;
struct IDxcIncludeHandler;

enum class ShaderType
{
    Vertex = 0,
    Pixel,
    Compute
};

class Shader
{
public:
    Shader(std::wstring_view name, IDxcBlob* blob)
        : mName(name), mBlob(blob)
    { }

    ~Shader()
    {
        mBlob->Release();
    }

    inline IDxcBlob* GetBlob() const { return mBlob; }
    inline std::wstring_view GetName() const { return mName; }
private:
    std::wstring mName;
    IDxcBlob* mBlob = nullptr;
};

using ShaderHandle = Shader*;

using ShaderToken = std::pair<std::string_view, std::string_view>;
using ShaderTokens = std::vector<ShaderToken>;

class ShaderManager
{
public:
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;

    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    bool Startup();
    bool Shutdown();

    ShaderHandle GetShader(std::wstring_view shaderName, ShaderType type, std::wstring_view entry = L"main", uint32_t instanceID = 0, ShaderTokens tokens = {});

    static ShaderManager& Get()
    {
        static ShaderManager* instance = new ShaderManager();
        return *instance;
    }

private:
    ShaderManager() = default;

    bool GetSourceCode(std::wstring_view path, std::string& sourceCode);
    void ApplyTokens(ShaderTokens tokens, std::wstring_view shaderPath, std::string& sourceCode);
    IDxcBlob* CompileShader(std::string_view sourceCode, ShaderType type, std::wstring_view entry, std::wstring_view shaderPath);
    std::wstring_view GetShaderTargetProfile(ShaderType type) const;
    size_t GetShaderKey(std::wstring_view shaderName, uint32_t instanceID) const;

    HMODULE mDXCHandle = nullptr;
    IDxcLibrary* mLibrary = nullptr;
    IDxcCompiler* mCompiler = nullptr;
    IDxcIncludeHandler* mIncludeHandler = nullptr;

    std::map<size_t, std::unique_ptr<Shader>> mShaders;

};
