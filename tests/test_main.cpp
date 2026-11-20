// GTest 会自动提供 main 函数（因为我们链接了 GTest::Main）
// 所以这个文件其实可以没有。但留着作为一个明确的入口点。
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    // 初始化 Google Test
    ::testing::InitGoogleTest(&argc, argv);
    // 运行所有 TEST 宏定义的测试
    return RUN_ALL_TESTS();
}