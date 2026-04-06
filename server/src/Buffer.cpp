#include "Buffer.hpp"
#include <cstdio>
#include <stdexcept>
#include <cstring>

Buffer::Buffer(): capacity_{10}, size_{0}{
    array_ = new uint8_t[capacity_];
}

Buffer::~Buffer(){
    delete[] array_;
}

Buffer& Buffer::operator<<(uint32_t num){
    checkCapacityFor(4);
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&num);
    memcpy(array_ + size_, ptr, 4);
    size_ += 4;
    return *this;
}

Buffer& Buffer::operator<<(uint8_t num){
    checkCapacityFor(1);
    array_[size_++] = num;
    return *this;
}

Buffer& Buffer::operator<<(char num){
    return *this << static_cast<uint8_t>(num); 
}

Buffer& Buffer::operator<<(const string& str){
    checkCapacityFor(str.size());
    memcpy(array_ + size_, str.data(), str.length());
    size_ += str.length();
    return *this;
}

void Buffer::checkCapacityFor(uint32_t num){
    if (capacity_ - size_ >= num) return;

    capacity_ *= 2;
    uint8_t* temp = reinterpret_cast<uint8_t*>(realloc(array_, capacity_));
    if (temp == nullptr)
        throw new runtime_error("not enough memory");

    delete[] array_;
    array_ = temp;
}

const uint8_t* Buffer::data(){
    return array_;
}

uint32_t Buffer::size(){
    return size_;
}

void Buffer::shrink(){
    if (size_ == 1){
        capacity_ = 1;
        return;
    }
    capacity_ = size_;
    uint8_t* temp = reinterpret_cast<uint8_t*>(realloc(array_, capacity_));
    if (temp == nullptr)
        throw runtime_error("not enough memory");
    
    delete[] array_;
    array_ = temp;
}