// ============================================================
// RingBuffer.h — 环形缓冲区（固定容量 FIFO 队列）
// ============================================================
// 一个线程安全的环形缓冲区，用于存储时间序列数据（如某个标签的历史值）。
// 特性：
//   容量固定，写满后新的写入会覆盖最旧的元素
//   支持按时间顺序读出全部元素、读取最新元素、清空、查询大小/容量
//   内部使用互斥锁保护，可在多线程环境下安全读写
// ============================================================
#pragma once

#include <vector>
#include <mutex>

template<typename T>
class RingBuffer
{
public:
    // 构造：capacity 为缓冲区最大容量（默认 600）
    explicit RingBuffer(size_t capacity = 600)
        : m_buffer(capacity)      // 预分配容量大小的存储空间
        , m_capacity(capacity)    // 记录最大容量
        , m_head(0)               // 写入位置从 0 开始
        , m_size(0)               // 初始元素个数为 0
    {
    }

    // 向缓冲区添加一个元素，如果缓冲区已满则覆盖最旧的元素
    void push(const T& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 将新值写入当前写入位置
        m_buffer[m_head] = value;

        // 写入位置前移（取模实现环形）
        m_head = (m_head + 1) % m_capacity;

        // 未满时元素个数递增；已满时个数保持为容量
        if (m_size < m_capacity) {
            ++m_size;
        }
    }

    // 从缓冲区读取所有元素，按时间顺序（最旧→最新）返回一个 std::vector
    std::vector<T> readAll() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<T> result;
        result.reserve(m_size);   // 预分配，减少动态扩容

        if (m_size == 0) return result;

        // 未写满时从下标 0 开始读；已写满时从最旧的元素（即 head）开始读
        size_t index = (m_size < m_capacity) ? 0 : m_head;

        for (size_t i = 0; i < m_size; ++i) {
            result.push_back(m_buffer[index]);
            index = (index + 1) % m_capacity;
        }

        return result;
    }

    // 获取缓冲区中最近的元素，如果缓冲区为空则返回默认构造的 T
    T latest() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_size == 0) return T();
        // 最新元素位于写入位置的前一个下标（注意环形回绕）
        size_t idx = (m_head == 0) ? (m_capacity - 1) : (m_head - 1);
        return m_buffer[idx];
    }

    // 获取当前缓冲区中元素的数量
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    // 获取缓冲区的最大容量
    size_t capacity() const
    {
        return m_capacity;
    }

    // 清空缓冲区（不释放内存，仅重置计数）
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_size = 0;
    }

private:
    std::vector<T> m_buffer;    // 环形缓冲区的存储
    size_t m_capacity;          // 缓冲区的最大容量
    size_t m_head;              // 下一个写入位置的索引
    size_t m_size;              // 当前缓冲区中元素的数量
    mutable std::mutex m_mutex; // 保护缓冲区访问的互斥锁（mutable 允许在 const 方法中加锁）
};
