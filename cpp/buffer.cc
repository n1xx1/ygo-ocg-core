#include "buffer.h"

BufferReader::BufferReader(void* buffer, uint32_t bufferLength)
    : buffer(buffer), bufferLength(bufferLength), index(0) {}

BufferReader BufferReader::subReader(int length, bool* eof) {
    if (index + length > bufferLength) {
        *eof = true;
        return BufferReader(nullptr, 0);
    }
    BufferReader ret(current(), length);
    index += length;
    return ret;
}

uint8_t* BufferReader::read(uint32_t size, bool* eof) {
    if (index + size > bufferLength) {
        *eof = true;
        return nullptr;
    }
    uint8_t* ret = reinterpret_cast<uint8_t*>(current());
    index += size;
    return ret;
}

template <> int8_t BufferReader::read<int8_t>(bool* eof) {
    if (index + 1 > bufferLength) {
        *eof = true;
        return 0;
    }
    int8_t value = *reinterpret_cast<int8_t*>(current());
    index += 1;
    return value;
}
template <> int16_t BufferReader::read<int16_t>(bool* eof) {
    if (index + 2 > bufferLength) {
        *eof = true;
        return 0;
    }
    int16_t value = *reinterpret_cast<int16_t*>(current());
    index += 2;
    return value;
}
template <> int32_t BufferReader::read<int32_t>(bool* eof) {
    if (index + 4 > bufferLength) {
        *eof = true;
        return 0;
    }
    int32_t value = *reinterpret_cast<int32_t*>(current());
    index += 4;
    return value;
}
template <> int64_t BufferReader::read<int64_t>(bool* eof) {
    if (index + 8 > bufferLength) {
        *eof = true;
        return 0;
    }
    int64_t value = *reinterpret_cast<int64_t*>(current());
    index += 8;
    return value;
}

template <> uint8_t BufferReader::read<uint8_t>(bool* eof) {
    if (index + 1 > bufferLength) {
        *eof = true;
        return 0;
    }
    uint8_t value = *reinterpret_cast<uint8_t*>(current());
    index += 1;
    return value;
}
template <> uint16_t BufferReader::read<uint16_t>(bool* eof) {
    if (index + 2 > bufferLength) {
        *eof = true;
        return 0;
    }
    uint16_t value = *reinterpret_cast<uint16_t*>(current());
    index += 2;
    return value;
}
template <> uint32_t BufferReader::read<uint32_t>(bool* eof) {
    if (index + 4 > bufferLength) {
        *eof = true;
        return 0;
    }
    uint32_t value = *reinterpret_cast<uint32_t*>(current());
    index += 4;
    return value;
}
template <> uint64_t BufferReader::read<uint64_t>(bool* eof) {
    if (index + 8 > bufferLength) {
        *eof = true;
        return 0;
    }
    uint64_t value = *reinterpret_cast<uint64_t*>(current());
    index += 8;
    return value;
}