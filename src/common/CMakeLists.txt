# ============================================================
# common 库
# 编译为静态库（.lib），被 core 和 app 引用
# 包含日志系统、配置管理、常用工具函数
# ============================================================
add_library(common STATIC
    Logger.cpp
    Logger.h
    ConfigManager.cpp
    ConfigManager.h
    Utility.h
)

# target_link_libraries：这个库需要链接哪些其他库
target_link_libraries(common PUBLIC
    Qt6::Core                     
    spdlog::spdlog                
    nlohmann_json::nlohmann_json  
)

# target_include_directories：指定头文件搜索路径
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})