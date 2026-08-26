#pragma once

#include <vector>

class MemoryPool
{
public:
    static MemoryPool& Instance()
    {
        static MemoryPool instance;
        return instance;
    }

    void Clear() { _buffer.clear(); _offset = 0; }
    std::vector<float>& GetData() { return _buffer; }
    void Resize(size_t size) { _buffer.resize(size); }
    size_t Size() const { return _buffer.size(); }

    void AddVectorToBuffer(const std::vector<float>& v);
    void GetVectorFromBuffer(std::vector<float>& v);

private:
    MemoryPool() = default;

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    std::vector<float> _buffer;
    size_t _offset = 0;
};
