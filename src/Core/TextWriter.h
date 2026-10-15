#pragma once
#include "BuildConfig.h"

#if !NICE_GFX_NATIVE_IDF
#include <Print.h>
namespace nice_gfx { using PrintBase = ::Print; }
#else
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <type_traits>

namespace nice_gfx {

// Native byte-stream formatting for display text. No Arduino types or global
// compatibility symbols are exposed by the ESP-IDF build.
class TextWriter {
public:
    virtual ~TextWriter() = default;
    virtual size_t write(uint8_t value) = 0;
    virtual size_t write(const uint8_t *data, size_t size) {
        if (!data) return 0;
        size_t written = 0;
        while (written < size && write(data[written]) == 1) ++written;
        return written;
    }
    size_t write(const char *text) {
        return text ? write(reinterpret_cast<const uint8_t *>(text), strlen(text)) : 0;
    }
    size_t print(const char *text) { return write(text); }
    size_t print(const std::string &text) {
        return write(reinterpret_cast<const uint8_t *>(text.data()), text.size());
    }
    size_t print(char value) { return write(uint8_t(value)); }
    size_t print(unsigned char value, int base = 10) { return printUnsigned(value, base); }
    size_t print(int value, int base = 10) { return printSigned(value, base); }
    size_t print(unsigned int value, int base = 10) { return printUnsigned(value, base); }
    size_t print(long value, int base = 10) { return printSigned(value, base); }
    size_t print(unsigned long value, int base = 10) { return printUnsigned(value, base); }
    size_t print(long long value, int base = 10) { return printSigned(value, base); }
    size_t print(unsigned long long value, int base = 10) { return printUnsigned(value, base); }
    size_t print(double value, int digits = 2) {
        char text[96];
        if (digits < 0) digits = 0;
        if (digits > 20) digits = 20;
        const int length = snprintf(text, sizeof(text), "%.*f", digits, value);
        if (length < 0) return 0;
        if (size_t(length) >= sizeof(text)) return write("ovf");
        return write(reinterpret_cast<const uint8_t *>(text), size_t(length));
    }
    size_t println() { return write("\r\n"); }
    template <typename T> size_t println(const T &value) {
        const size_t written = print(value);
        return written + println();
    }
    template <typename T> size_t println(const T &value, int format) {
        const size_t written = print(value, format);
        return written + println();
    }

private:
    template <typename T> size_t printSigned(T value, int base) {
        if (base == 10 && value < 0) {
            const size_t sign = write(uint8_t('-'));
            return sign + printUnsigned(0ULL - static_cast<unsigned long long>(value), base);
        }
        using Unsigned = typename std::make_unsigned<T>::type;
        return printUnsigned(static_cast<Unsigned>(value), base);
    }
    size_t printUnsigned(unsigned long long value, int base) {
        if (base == 0) return write(uint8_t(value));
        if (base < 2 || base > 36) base = 10;
        char text[sizeof(value) * 8];
        char *cursor = text + sizeof(text);
        do {
            const unsigned digit = unsigned(value % unsigned(base));
            *--cursor = char(digit < 10 ? '0' + digit : 'A' + digit - 10);
            value /= unsigned(base);
        } while (value);
        return write(reinterpret_cast<const uint8_t *>(cursor), size_t(text + sizeof(text) - cursor));
    }
};

using PrintBase = TextWriter;
} // namespace nice_gfx
#endif
