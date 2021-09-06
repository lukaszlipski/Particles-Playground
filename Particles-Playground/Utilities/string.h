#pragma once

std::string ConvertWStringToString(std::wstring_view str)
{
    int32_t size = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring ConvertStringToWString(std::string_view str)
{
    int count = MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int32_t>(str.size()), nullptr, 0);
    std::wstring result(count, 0);
    MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int32_t>(str.size()), result.data(), count);
    return result;
}
