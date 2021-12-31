#pragma once

#include <iostream>
#include <memory>

namespace tinydhcpd
{
    //from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
    template<typename ... Args>
    std::string string_format(const std::string& format, Args ... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
        auto size = static_cast<size_t>(size_s);
        auto buf = std::make_unique<char[]>(size);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    template<typename N, size_t size>
    std::array<uint8_t, size> to_byte_array(N number)
    {
        std::array<uint8_t, size> arr;
        for (size_t i = 0; i < size; i++)
        {
            uint8_t shift = 8 * i;
            arr[i] = (number & (((N)0xff) << shift)) >> shift;
        }
        return arr;
    }

    std::array<uint8_t, 4> to_network_byte_array(uint32_t number);
    std::array<uint8_t, 2> to_network_byte_array(uint16_t number);
} // namespace tinydhcpd