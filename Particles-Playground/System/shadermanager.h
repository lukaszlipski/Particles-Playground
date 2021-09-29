#pragma once
#include "System/shader.h"

struct IDxcLibrary;
struct IDxcCompiler;
struct IDxcBlob;
struct IDxcIncludeHandler;

// Global shaders
extern ShaderHandle VS_Screen;
extern ShaderHandle PS_Screen;
extern ShaderHandle VS_DrawParticle;
extern ShaderHandle PS_DrawParticle;
extern ShaderHandle CS_ResetFreeIndices;
extern ShaderHandle CS_EmitterUpdate;

using ShaderToken = std::pair<std::string_view, std::string_view>;
using ShaderTokens = std::vector<ShaderToken>;

class ShaderCompilationResult
{
public:
    ShaderCompilationResult(ShaderHandle handle)
        : mHandle(handle)
    { }

    ShaderCompilationResult(std::string errorMsg)
        : mErrorMsg(std::move(errorMsg))
    { }

    ShaderCompilationResult(const ShaderCompilationResult&) = default;
    ShaderCompilationResult(ShaderCompilationResult&&) = default;
    ShaderCompilationResult& operator=(const ShaderCompilationResult&) = default;
    ShaderCompilationResult& operator=(ShaderCompilationResult&&) = default;

    ~ShaderCompilationResult() = default;

    inline bool IsValid() const { return mHandle.GetHandle() != ShaderHandle::Invalid; }
    inline ShaderHandle GetHandle() const { Assert(IsValid()); return mHandle; }
    inline std::string_view GetError() const { Assert(!IsValid()); return mErrorMsg; }

private:
    ShaderHandle mHandle;
    std::string mErrorMsg;
};

class ShaderManager
{
    static const uint32_t MaxShaders = 64;

public:
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;

    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    bool Startup();
    bool Shutdown();

    ShaderCompilationResult CompileShader(std::wstring_view shaderName, ShaderType type, std::wstring_view entry = L"main", ShaderTokens tokens = {});
    inline Shader* GetShader(ShaderHandle handle) { return mShadersPool.GetObject(handle); }
    inline void FreeShader(ShaderHandle handle) { mShadersPool.FreeObject(handle); }

    static ShaderManager& Get()
    {
        static ShaderManager* instance = new ShaderManager();
        return *instance;
    }

private:
    ShaderManager()
        : mShadersPool(MaxShaders)
    { }

    bool GetSourceCode(std::wstring_view path, std::string& sourceCode);
    void ApplyTokens(ShaderTokens tokens, std::wstring_view shaderPath, std::string& sourceCode);
    IDxcBlob* CompileShader(std::string_view sourceCode, ShaderType type, std::wstring_view entry, std::wstring_view shaderPath, std::string& error);
    std::wstring_view GetShaderTargetProfile(ShaderType type) const;

    HMODULE mDXCHandle = nullptr;
    IDxcLibrary* mLibrary = nullptr;
    IDxcCompiler* mCompiler = nullptr;
    IDxcIncludeHandler* mIncludeHandler = nullptr;

    ObjectPool<Shader> mShadersPool;

};
