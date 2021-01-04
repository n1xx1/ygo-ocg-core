#pragma once

#include <cstdint>
#include <tuple>

class BufferReader {
  public:
    BufferReader(void* buffer, uint32_t bufferLength);
    BufferReader subReader(int length, bool* eof);
    template <typename T> T read(bool* eof);
    uint8_t* read(uint32_t size, bool* eof);
    inline uint32_t avail() { return bufferLength - index; }

  private:
    inline void* current() { return reinterpret_cast<char*>(buffer) + index; }

    void* buffer;
    uint32_t bufferLength;
    uint32_t index;
};
