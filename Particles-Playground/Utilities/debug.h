#pragma once

template<typename ...Args>
void OutputDebugMessage(std::string_view format, Args... args)
{
    const int32_t size = std::snprintf(nullptr, 0, format.data(), args...);
    std::vector<char> buf(size + 1);
    std::snprintf(buf.data(), buf.size(), format.data(), args...);
    OutputDebugStringA(buf.data());
}
