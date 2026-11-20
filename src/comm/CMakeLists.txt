# ============================================================
# comm 模块构建脚本
# ============================================================
#
# 本目录对应“通信模块”，负责定义通信接口、协议实现与通信管理，
# 是系统与外部设备（PLC、仪表、仿真引擎等）进行数据交互的桥梁。
#
# 构建产物：
#   静态库 comm.lib（或 libcomm.a），供上层 app 模块链接使用。
#
# 源文件组成：
#   - ICommunicationPort.h        通信端口抽象接口（仅头文件）
#   - CommManager.cpp / .h        通信管理器（单例，统一调度所有设备）
#   - modbus/ModbusMapping.*       Modbus 位号 → 寄存器地址映射
#   - modbus/ModbusRtuPort.*       Modbus RTU（串口）通信端口
#   - modbus/ModbusTcpPort.*       Modbus TCP（网络）通信端口
# ============================================================

# 声明一个静态库目标，库名为 comm。
# STATIC 表示编译为静态库；列出参与编译的源文件与头文件。
add_library(comm STATIC
    ICommunicationPort.h
    CommManager.cpp
    CommManager.h
    modbus/ModbusMapping.cpp 
    modbus/ModbusMapping.h
    modbus/ModbusRtuPort.cpp 
    modbus/ModbusRtuPort.h
    modbus/ModbusTcpPort.cpp 
    modbus/ModbusTcpPort.h
)

# 链接本模块所需的依赖库。
# PUBLIC 表示这些依赖会同时传递给链接 comm 的目标（如 app）。
target_link_libraries(comm PUBLIC
    core              # 核心模块（实时数据库、标签管理等）
    common            # 公共模块（日志、配置、工具等）
    Qt6::Core         # Qt 核心（信号槽、事件循环、字符串等）
    Qt6::SerialPort   # Qt 串口通信（Modbus RTU 需要）
    Qt6::SerialBus    # Qt 现场总线（提供 QModbusDevice 等 Modbus 支持）
    Qt6::Network      # Qt 网络通信（Modbus TCP/UDP 需要）
)

# 将当前源码目录加入头文件搜索路径，使模块内可用
# #include "CommManager.h" 等相对引用方式直接包含头文件。
target_include_directories(comm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
