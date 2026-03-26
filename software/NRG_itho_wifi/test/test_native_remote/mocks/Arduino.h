// Minimal Arduino.h mock for native unit testing
#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>

// Type aliases that Arduino code expects
typedef uint8_t byte;

// String copy helper
inline size_t strlcpy(char *dst, const char *src, size_t size) {
    size_t len = strlen(src);
    if (size > 0) {
        size_t copy = (len >= size) ? size - 1 : len;
        memcpy(dst, src, copy);
        dst[copy] = '\0';
    }
    return len;
}

// Minimal String class stub
class String {
public:
    String() : _buf() {}
    String(const char *s) : _buf(s ? s : "") {}
    String(int val) : _buf(std::to_string(val)) {}
    const char *c_str() const { return _buf.c_str(); }
    unsigned int length() const { return _buf.length(); }
    bool isEmpty() const { return _buf.empty(); }
    bool operator==(const String &other) const { return _buf == other._buf; }
    bool operator==(const char *other) const { return _buf == other; }
private:
    std::string _buf;
};
