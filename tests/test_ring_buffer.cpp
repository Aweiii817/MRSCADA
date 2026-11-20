// ============================================================
// RingBuffer 单元测试
//
// 测试内容：
//   1. 空缓冲区正确返回空
//   2. 推入和读取顺序一致
//   3. latest() 返回最新值
//   4. 溢出时自动覆盖最旧数据
//   5. clear() 正确清空
//   6. 大量写入时容量稳定
// ============================================================
#include <gtest/gtest.h>
#include "core/rtdb/RingBuffer.h"

// 测试 1：新建的缓冲区应该是空的
TEST(RingBufferTest, ShouldBeEmptyInitially)
{
    RingBuffer<int> buf(5);    // 容量为 5
    EXPECT_EQ(buf.size(), 0);               // 大小应为 0
    EXPECT_TRUE(buf.readAll().empty());       // 读取结果应为空
}

// 测试 2：推入数据后，readAll 应保持顺序
TEST(RingBufferTest, ShouldPreserveOrder)
{
    RingBuffer<int> buf(5);
    buf.push(10);
    buf.push(20);
    buf.push(30);

    EXPECT_EQ(buf.size(), 3);               // 现在有 3 个元素
    auto data = buf.readAll();
    ASSERT_EQ(data.size(), 3);               // 读取到 3 个
    EXPECT_EQ(data[0], 10);                  // 最先推入的在索引 0
    EXPECT_EQ(data[1], 20);
    EXPECT_EQ(data[2], 30);                  // 最后推入的在索引 2
}

// 测试 3：latest() 返回最新推入的值
TEST(RingBufferTest, ShouldReturnLatestValue)
{
    RingBuffer<int> buf(5);
    buf.push(100);
    EXPECT_EQ(buf.latest(), 100);   // 最新值 = 100
    buf.push(200);
    EXPECT_EQ(buf.latest(), 200);   // 最新值 = 200
}

// 测试 4：推入超过容量的数据，最旧的应该被覆盖
TEST(RingBufferTest, ShouldOverwriteWhenFull)
{
    RingBuffer<int> buf(3);  // 容量只有 3
    buf.push(1);    // [1]
    buf.push(2);    // [1, 2]
    buf.push(3);    // [1, 2, 3] ← 满了
    buf.push(4);    // [4, 2, 3] ← 1 被覆盖

    EXPECT_EQ(buf.size(), 3);            // 容量仍是 3
    auto data = buf.readAll();
    ASSERT_EQ(data.size(), 3);
    EXPECT_EQ(data[0], 2);               // 现在最旧的是 2
    EXPECT_EQ(data[1], 3);
    EXPECT_EQ(data[2], 4);               // 最新的是 4
}

// 测试 5：清空后重新变为空
TEST(RingBufferTest, ShouldBeEmptyAfterClear)
{
    RingBuffer<int> buf(5);
    buf.push(1);
    buf.push(2);
    buf.clear();
    EXPECT_EQ(buf.size(), 0);
    EXPECT_TRUE(buf.readAll().empty());
}

// 测试 6：大量写入测试——验证容量稳定
TEST(RingBufferTest, ShouldHandleLargeNumberOfWrites)
{
    RingBuffer<int> buf(1000);    // 容量 1000
    for (int i = 0; i < 10000; ++i) {
        buf.push(i);               // 写入 10000 个
    }
    EXPECT_EQ(buf.size(), 1000);   // 容量仍是 1000
    auto data = buf.readAll();
    EXPECT_EQ(data[0], 9000);      // 最旧的是 9000（10000-1000）
    EXPECT_EQ(data[999], 9999);    // 最新的是 9999
}