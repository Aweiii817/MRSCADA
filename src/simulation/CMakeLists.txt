# ============================================================
# simulation — 仿真模块（模拟反应釜的生产工艺过程）
#
# 本模块提供一个"虚拟设备"，在没有真实 PLC / 仪表接入时，
# 用数学模型驱动整个上位机系统运行，便于开发调试与演示。
# 它内部包含三个核心类：
#   SimEngine       仿真引擎：用 QTimer 周期性地推进模型，
#                   并把计算结果发布到实时数据库。
#   SimReactorModel 反应釜数学模型：模拟温度、压力、液位、
#                   进料流量、阀门开度等过程变量。
#   SimPort         仿真通信端口：实现 ICommunicationPort
#                   接口，让仿真数据以"设备"的形式接入通信管理器。
#
# 编译产物为静态库，供 app 模块链接。
# ============================================================

# ------------------------------------------------------------
# 声明静态库 simulation，并列出其源文件 / 头文件
# ------------------------------------------------------------
add_library(simulation STATIC
    SimEngine.cpp          # 仿真引擎实现
    SimEngine.h            # 仿真引擎声明（QTimer 定时推进模型）
    SimReactorModel.cpp    # 反应釜数学模型实现
    SimReactorModel.h      # 反应釜数学模型声明（过程变量快照）
    SimPort.cpp            # 仿真通信端口实现
    SimPort.h              # 仿真通信端口声明（实现 ICommunicationPort 接口）
)

# ------------------------------------------------------------
# 声明 simulation 库的依赖（PUBLIC：依赖会传递给链接者 app）
# ------------------------------------------------------------
target_link_libraries(simulation PUBLIC
    core       # SimEngine 需要 RealTimeDB 发布数据、TagDataPoint 位号定义
    common     # 需要 Logger 日志、ConfigManager 配置
    comm       # SimPort 需要 ICommunicationPort / DeviceConfig / DeviceStatus 接口定义
    Qt6::Core  # QObject 信号槽、QTimer 定时器
)

# ------------------------------------------------------------
# 声明头文件搜索路径：让 app 等模块可直接包含本目录下的头文件
# ------------------------------------------------------------
target_include_directories(simulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
