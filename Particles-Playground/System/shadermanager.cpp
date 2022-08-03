#include "shadermanager.h"
#include "Utilities/debug.h"
#include "Utilities/string.h"
#include "System/graphic.h"

const std::wstring SHADER_SOURCE_FOLDER = L"Shaders/";

ShaderHandle VS_Screen;
ShaderHandle PS_Screen;
ShaderHandle VS_DrawParticle;
ShaderHandle PS_DrawParticle;
ShaderHandle CS_ResetFreeIndices;
ShaderHandle CS_EmitterUpdate;

bool ShaderManager::Startup()
{
    mDXCHandle = LoadLibrary(L"dxcompiler.dll");
    if (!mDXCHandle) { return false; }

    DxcCreateInstanceProc DxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(mDXCHandle, "DxcCreateInstance");

    if (!DxcCreateInstance) { return false; }

    if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mLibrary)))) { return false; }
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompiler)))) { return false; }

    if (FAILED(mLibrary->CreateIncludeHandler(&mIncludeHandler))) { return false; }

    mShadersPool.Init();

    VS_Screen = CompileShader(L"vsscreen", ShaderType::Vertex).GetHandle();
    PS_Screen = CompileShader(L"psscreen", ShaderType::Pixel).GetHandle();
    VS_DrawParticle = CompileShader(L"vsdefault", ShaderType::Vertex).GetHandle();
    PS_DrawParticle = CompileShader(L"psdefault", ShaderType::Pixel).GetHandle();
    CS_ResetFreeIndices = CompileShader(L"resetfreeindices", ShaderType::Compute).GetHandle();
    CS_EmitterUpdate = CompileShader(L"emitterupdate", ShaderType::Compute).GetHandle();

    return true;
}

bool ShaderManager::Shutdown()
{
    FreeShader(VS_Screen);
    FreeShader(PS_Screen);
    FreeShader(VS_DrawParticle);
    FreeShader(PS_DrawParticle);
    FreeShader(CS_ResetFreeIndices);
    FreeShader(CS_EmitterUpdate);

    mShadersPool.Free();

    mIncludeHandler->Release();

    mLibrary->Release();
    mCompiler->Release();

    FreeLibrary(mDXCHandle);

    return true;
}

ShaderCompilationResult ShaderManager::CompileShader(std::wstring_view shaderName, ShaderType type, std::wstring_view entry, ShaderTokens tokens)
{
    Assert(shaderName.size());

    const std::wstring shaderPath = SHADER_SOURCE_FOLDER + shaderName.data() + L".hlsl";

    std::string sourceCode;
    if (!GetSourceCode(shaderPath, sourceCode)) { return ShaderCompilationResult("Can't find a shader file"); }

    ApplyTokens(tokens, shaderPath, sourceCode);

    std::string errorMsg;
    IDxcBlob* shaderBlob = CompileShader(sourceCode, type, entry, shaderPath, errorMsg);

    if (!shaderBlob) { return ShaderCompilationResult(std::move(errorMsg)); }

    return ShaderCompilationResult(mShadersPool.AllocateObject(shaderName, type, shaderBlob));
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
            OutputDebugMessage("Can't find a token (%s) inside shader ", token.first, shaderPathA.data());
            continue;
        }

        sourceCode.replace(start, size, token.second);
    }
}

IDxcBlob* ShaderManager::CompileShader(std::string_view sourceCode, ShaderType type, std::wstring_view entry, std::wstring_view shaderPath, std::string& errorMsg)
{
    errorMsg.clear();

    IDxcBlobEncoding* sourceBlob = nullptr;
    mLibrary->CreateBlobWithEncodingFromPinned(sourceCode.data(), static_cast<uint32_t>(sourceCode.size()), CP_UTF8, &sourceBlob);

    std::array defines = { 
        DxcDefine{ L"ENABLE_RESOURCE_DESCRIPTOR_HEAP", Graphic::Get().SupportsResourceDescriptorHeap() ? L"1" : L"0" }
    };

    IDxcOperationResult* result = nullptr;
    Assert(SUCCEEDED(mCompiler->Compile(sourceBlob, shaderPath.data(), entry.data(), GetShaderTargetProfile(type).data(), 
        nullptr, 0, defines.data(), static_cast<uint32_t>(defines.size()), mIncludeHandler, &result)));

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

        errorMsg = ConvertWStringToString(shaderPath);
        const char* error = static_cast<const char*>(errorsBlob->GetBufferPointer());
        OutputDebugMessage("Shader compilation error: %s.hlsl\n%s", errorMsg.data(), error);

        errorsBlob->Release();
    }

    result->Release();

    return blob;
}

std::wstring_view ShaderManager::GetShaderTargetProfile(ShaderType type) const
{
    std::wstring_view vertexProfiles[] = { L"vs_6_0", L"vs_6_1", L"vs_6_2", L"vs_6_3", L"vs_6_4", L"vs_6_5", L"vs_6_6" };
    std::wstring_view pixelProfiles[] = { L"ps_6_0", L"ps_6_1", L"ps_6_2", L"ps_6_3", L"ps_6_4", L"ps_6_5", L"ps_6_6" };
    std::wstring_view computeProfiles[] = { L"cs_6_0", L"cs_6_1", L"cs_6_2", L"cs_6_3", L"cs_6_4", L"cs_6_5", L"cs_6_6" };

    D3D_SHADER_MODEL sm = Graphic::Get().GetSM();
    switch (type)
    {
    case ShaderType::Vertex: return vertexProfiles[sm - D3D_SHADER_MODEL_6_0];
    case ShaderType::Pixel: return pixelProfiles[sm - D3D_SHADER_MODEL_6_0];
    case ShaderType::Compute: return computeProfiles[sm - D3D_SHADER_MODEL_6_0];
    default: return L"";
    }
}
