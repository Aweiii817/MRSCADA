#include <gtest/gtest.h>
#include "simulation/SimReactorModel.h"
#include <QCoreApplication>
#include "simulation/SimEngine.h"

// 测试：初始状态应为室温常压空釜
TEST(SimReactorTest, ShouldStartAtInitialState)
{
    SimReactorModel reactor;
    const auto& state = reactor.getState();

    EXPECT_NEAR(state.temperature, 25.0, 0.1);  // 室温
    EXPECT_NEAR(state.pressure, 0.1, 0.01);     // 常压
    EXPECT_NEAR(state.level, 0.0, 0.1);          // 空釜
}

// 测试：加热阶段温度上升
TEST(SimReactorTest, ShouldHeatUp)
{
    SimReactorModel reactor;
    reactor.setValve("TV-101", 1.0);   // 蒸汽阀全开
    reactor.setStage(ProcessStage::HEATING);

    double initial_temp = reactor.getState().temperature;
    for (int i = 0; i < 50; ++i) {
        reactor.step(0.1);  // 步进 5 秒
    }

    EXPECT_GT(reactor.getState().temperature, initial_temp);
}

// 测试：进料阶段液位上升
TEST(SimReactorTest, ShouldFillUp)
{
    SimReactorModel reactor;
    reactor.setValve("HV-101", 1.0);   // 进料阀全开
    reactor.setStage(ProcessStage::FEEDING);

    for (int i = 0; i < 30; ++i) {
        reactor.step(0.1);  // 步进 3 秒
    }

    EXPECT_GT(reactor.getState().level, 0.0);
}

TEST(SimEngineTest, ShouldStartAndStop)
{
    int argc = 1;
    char* argv[] = { const_cast<char*>("test") };
    QCoreApplication app(argc, argv);

    SimEngine engine;
    EXPECT_FALSE(engine.isRunning());

    engine.start(50);   // 50ms 步进
    EXPECT_TRUE(engine.isRunning());

    // 等待 200ms 让引擎运行几步
    QTimer::singleShot(200, &engine, &SimEngine::stop);

    // 启动事件循环（200ms 后自动退出）
    QTimer::singleShot(300, &app, &QCoreApplication::quit);
    app.exec();

    EXPECT_FALSE(engine.isRunning());
}