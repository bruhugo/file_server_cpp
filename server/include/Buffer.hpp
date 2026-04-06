#pragma once
#include <cstdint>
#include <string>

using namespace std;

class Buffer {
public:
    Buffer();
    ~Buffer();

    Buffer& operator<<(uint32_t);
    Buffer& operator<<(uint8_t);
    Buffer& operator<<(char);
    Buffer& operator<<(const string&);

    template<typename T>
    Buffer& operator<<(T val){
        size_t sT = sizeof(T);
        checkCapacityFor(sT);
        uint8_t *ptr = reinterpret_cast<uint8_t*>(val);
        memcpy(array_ + size_, ptr, sT);
        size_ += sT;
        return *ptr;
    }

    const uint8_t* data();
    uint32_t size();
    void shrink();

private: 
    // checks if dynamic container has space
    // for the given number of bytes
    void checkCapacityFor(uint32_t size);

    uint32_t capacity_;
    uint32_t size_;
    uint8_t* array_;
};

