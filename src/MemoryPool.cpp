#include "MemoryPool.h"

#include <stdexcept>

void MemoryPool::AddVectorToBuffer(const std::vector<float>& v)
{
    _buffer.insert(_buffer.end(), v.begin(), v.end());
}

void MemoryPool::GetVectorFromBuffer(std::vector<float>& v)
{
    const size_t count = v.size();

    if (_offset + count > _buffer.size())
        throw std::out_of_range("MemoryPool read exceeds buffer size");

    std::copy(
        _buffer.begin() + _offset,
        _buffer.begin() + _offset + count,
        v.begin()
    );

    _offset += count;
}
