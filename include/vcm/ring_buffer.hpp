#pragma once
#include<stdint.h>
#include<cstddef>
#include<atomic>

template <typename T,size_t capacity>//创建方法 RingBuffer<T,2的n次幂>

class RingBuffer{
    static_assert((capacity > 0) && ((capacity & (capacity >> 1)) == 0));
    public:
    bool push(T value){
        size_t r = read_idx.load(std::memory_order_acquire);
        size_t w = write_idx.load(std::memory_order_acquire);

        if(((w + 1) & MASK) == r)return false;// 缓冲区满，放弃写入

        buffer_[w] = value;// 将数据写入写索引位置
        write_idx.store((w + 1) & MASK,std::memory_order_release);// 更新写索引（循环寻址）
        return true;
    }

    bool pop(T& value){
       size_t r = read_idx.load(std::memory_order_acquire);
       size_t w = write_idx.load(std::memory_order_acquire);

       if(w == r) return false;

       value = buffer_[r];
       read_idx.store((r + 1) & MASK,std::memory_order_release);
       return true;
    }

    bool empty()const{
        size_t w = write_idx.load(std::memory_order_acquire);
        size_t r = read_idx.load(std::memory_order_acquire);
        return w == r;
    }

    bool full()const{
        size_t w = write_idx.load(std::memory_order_acquire);
        size_t r = read_idx.load(std::memory_order_acquire);
        return ((w + 1) & MASK )== r;
    }

    size_t size()const{
        size_t w = write_idx.load(std::memory_order_acquire);
        size_t r = read_idx.load(std::memory_order_acquire);
        return (w - r) & MASK;
    }
    
    private:
    T buffer_[capacity];
    
    std::atomic<size_t> write_idx{0};
    std::atomic<size_t> read_idx{0};

    static constexpr size_t MASK = capacity - 1;
};