#include "shadermanager.h"
#include "Utilities/debug.h"
#include "Utilities/string.h"

const std::wstring SHADER_SOURCE_FOLDER = L"Shaders/";

bool ShaderManager::Startup()
{
    mDXCHandle = LoadLibrary(L"dxcompiler.dll");
    if (!mDXCHandle) { return false; }

    DxcCreateInstanceProc DxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(mDXCHandle, "DxcCreateInstance");

    if (!DxcCreateInstance) { return false; }

    if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mLibrary)))) { return false; }
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompiler)))) { return false; }

    if (FAILED(mLibrary->CreateIncludeHandler(&mIncludeHandler))) { return false; }

    return true;
}

bool ShaderManager::Shutdown()
{
    mShaders.clear();

    mIncludeHandler->Release();

    mLibrary->Release();
    mCompiler->Release();

    FreeLibrary(mDXCHandle);

    return true;
}

ShaderHandle ShaderManager::GetShader(std::wstring_view shaderName, ShaderType type, std::wstring_view entry, uint32_t instanceID, ShaderTokens tokens)
{
    Assert(shaderName.size());

    const size_t key = GetShaderKey(shaderName, instanceID);
    auto shaderIt = mShaders.find(key);

    if (shaderIt != mShaders.end())
    {
        return shaderIt->second.get();
    }

    const std::wstring shaderPath = SHADER_SOURCE_FOLDER + shaderName.data() + L".hlsl";

    std::string sourceCode;
    if (!GetSourceCode(shaderPath, sourceCode)) { return nullptr; }

    ApplyTokens(tokens, shaderPath, sourceCode);

    IDxcBlob* shaderBlob = CompileShader(sourceCode, type, entry, shaderPath);

    if (!shaderBlob) { return nullptr; }

    std::unique_ptr shader = std::make_unique<Shader>(shaderName, shaderBlob);
    ShaderHandle handle = shader.get();
    mShaders[key] = std::move(shader);

    return handle;
}

bool ShaderManager::GetSourceCode(std::wstring_view path, std::string& sourceCode)
{
    HANDLE fileHandle = CreateFile(path.data(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (fileHandle == INVALID_HANDLE_VALUE) { return false; }

    DWORD size = GetFileSize(fileHandle, 0);
    sourceCode.resize(size);

    DWORD dataRead = {};
    ReadFile(fileHandle, sourceCode.data(), size, &dataRead, 0);
    Assert(dataRead == size);

    CloseHandle(fileHandle);

    return true;
}

void ShaderManager::ApplyTokens(ShaderTokens tokens, std::wstring_view shaderPath, std::string& sourceCode)
{
    for (const ShaderToken& token : tokens)
    {
        const size_t start = sourceCode.find(token.first);
        const size_t size = token.first.size();

        if (start == std::string::npos)
        {
            std::string shaderPathA = ConvertWStringToString(shaderPath);
            OutputDebugMessage("Can't find a token (%s) inside template ", token.first, shaderPathA.data());
            continue;
        }

        sourceCode.replace(start, size, token.second);
    }
}

IDxcBlob* ShaderManager::CompileShader(std::string_view sourceCode, ShaderType type, std::wstring_view entry, std::wstring_view shaderPath)
{
    IDxcBlobEncoding* sourceBlob = nullptr;
    mLibrary->CreateBlobWithEncodingFromPinned(sourceCode.data(), static_cast<uint32_t>(sourceCode.size()), CP_UTF8, &sourceBlob);
    
    IDxcOperationResult* result = nullptr;
    Assert(SUCCEEDED(mCompiler->Compile(sourceBlob, shaderPath.data(), entry.data(), GetShaderTargetProfile(type).data(), nullptr, 0, nullptr, 0, mIncludeHandler, &result)));

    HRESULT compilationResult;
    result->GetStatus(&compilationResult);

    IDxcBlob* blob = nullptr;
    if (SUCCEEDED(compilationResult))
    {
        Assert(SUCCEEDED(result->GetResult(&blob)));
    }
    else
    {
        IDxcBlobEncoding* errorsBlob = nullptr;
        Assert(SUCCEEDED(result->GetErrorBuffer(&errorsBlob)));

        std::string shaderPathA = ConvertWStringToString(shaderPath);
        const char* error = static_cast<const char*>(errorsBlob->GetBufferPointer());
        OutputDebugMessage("Shader compilation error: %s.hlsl\n%s", shaderPathA.data(), error);

        errorsBlob->Release();
    }

    result->Release();

    return blob;
}

std::wstring_view ShaderManager::GetShaderTargetProfile(ShaderType type) const
{
    switch (type)
    {
        case ShaderType::Vertex: return L"vs_6_0";
        case ShaderType::Pixel: return L"ps_6_0";
        case ShaderType::Compute: return L"cs_6_0";
        default: return L"";
    }
}

size_t ShaderManager::GetShaderKey(std::wstring_view shaderName, uint32_t instanceID) const
{
    return std::hash<std::wstring_view>{}(std::wstring(shaderName) + std::to_wstring(instanceID));
}
