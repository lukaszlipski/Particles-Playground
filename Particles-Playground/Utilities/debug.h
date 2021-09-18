#pragma once

// The __noop enables the use of bracket-less if-else statements and semicolon after the macro.
// if (condition)
//   Assert(false);
// else
//   ...
#define Assert(x) if (!(x)) { __debugbreak(); char* ptr = nullptr; *ptr = 0;  } else __noop

template<typename ...Args>
void OutputDebugMessage(std::string_view format, Args... args)
{
    const int32_t size = std::snprintf(nullptr, 0, format.data(), args...);
    std::vector<char> buf(size + 1);
    std::snprintf(buf.data(), buf.size(), format.data(), args...);
    OutputDebugStringA(buf.data());
}
