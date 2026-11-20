# ==============================================================================
# core 模块 — 核心数据层
# ==============================================================================
# 本模块以静态库（STATIC）形式构建，是整个系统数据能力的核心，向上层提供：
#   tag     位号/标签管理（TagManager）
#   rtdb    实时数据库（RealTimeDB + RingBuffer 环形缓冲）
#   history 历史数据存储（HistoryDB）
#   alarm   报警规则与报警引擎（AlarmRule / AlarmEvent / AlarmEngine）
# 依赖 common（日志、配置等公共能力）。
# ==============================================================================

# 定义静态库 core，并列出其源文件（按功能目录分组）：
#   tag/       位号管理相关
#   rtdb/      实时数据库相关
#   history/   历史数据相关
#   alarm/     报警相关
add_library(core STATIC
    tag/TagManager.cpp
    tag/TagManager.h
    tag/TagDataPoint.h
    rtdb/RealTimeDB.cpp
    rtdb/RealTimeDB.h
    rtdb/RingBuffer.h
    history/HistoryDB.cpp
    history/HistoryDB.h
    alarm/AlarmRule.h
    alarm/AlarmEvent.h
    alarm/AlarmEngine.h
    alarm/AlarmEngine.cpp
)

# 声明 core 的链接依赖：
#   PUBLIC 表示依赖会传递给所有链接 core 的目标
#   common                本项目的公共模块（日志、配置等）
#   Qt6::Core             Qt 核心库（信号槽、容器等）
#   Qt6::Sql              Qt 数据库模块（HistoryDB 等用到）
#   nlohmann_json          JSON 解析库（标签/配置等用到）
target_link_libraries(core PUBLIC
    common
    Qt6::Core
    Qt6::Sql
    nlohmann_json::nlohmann_json
)

# 将当前源码目录设为对外可见的头文件搜索路径
# PUBLIC 保证依赖 core 的目标也能直接 #include 本模块的头文件
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
