# ==============================================================================
# UCS 上位机系统 - 顶层 CMake 构建脚本
# 项目全称：Upper Computer System for Chemical Process Control（化工过程控制上位机）
# ==============================================================================

# 指定构建本项目所需的最低 CMake 版本。
# 要求 3.22 及以上，以保证下面用到的命令与特性均可用。
cmake_minimum_required(VERSION 3.22)

# 声明项目基本信息：
#   VERSION      项目版本号
#   DESCRIPTION  项目简要说明
#   LANGUAGES    使用的编程语言（本项目为 C++）
project(UCS
	VERSION 1.0.0 DESCRIPTION
	"Upper Computer System for Chemical Process Control"
	LANGUAGES
	CXX
)

# 使用 C++17 标准编译，并要求编译器必须支持该标准（不支持则报错）。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt 的自动预处理工具：
#   AUTOMOC 自动运行 moc，处理 Q_OBJECT 等元对象代码
#   AUTORCC 自动编译 .qrc 资源文件
#   AUTOUIC 自动处理 .ui 界面文件并生成对应头文件
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6 并指定本工程需要使用的模块：
#   Core          核心模块（信号槽、事件循环等基础能力）
#   Widgets       GUI 控件（QWidget、窗口等）
#   Sql           数据库访问（SQLite/MySQL 等）
#   Concurrent    多线程并发（QtConcurrent）
#   Test          单元测试（QTest）
#   SerialPort    串口通信
#   SerialBus     现场总线（如 Modbus、CAN）
#   Network       网络通信
#   Charts        图表绘制
#   PrintSupport  打印支持
find_package(Qt6 REQUIRED COMPONENTS
	Core
	Widgets
	Sql
	Concurrent
	Test
	SerialPort
	SerialBus
	Network
	Charts
	PrintSupport
)

# 查找第三方依赖库：
#   spdlog         高性能 C++ 日志库
#   nlohmann_json  JSON 解析库
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

# 将 src 目录加入头文件搜索路径，便于各模块用 #include "xxx.h" 直接引用源码头文件。
include_directories(${CMAKE_SOURCE_DIR}/src)

# 按依赖顺序添加各子模块目录（每个子目录内含自己的 CMakeLists.txt）：
#   comm        通信模块（串口/总线/网络等设备接入）
#   common      公共模块（通用工具、数据类型等）
#   core        核心模块（实时数据库、报警引擎、用户/标签管理等）
#   simulation  仿真模块（过程仿真引擎）
#   ui          UI 模块（各界面控件与视图）
#   app         应用程序入口（主程序可执行文件）
#   business    业务模块（配方等业务逻辑）
add_subdirectory(src/comm)
add_subdirectory(src/common)
add_subdirectory(src/core)
add_subdirectory(src/simulation)
add_subdirectory(src/ui)
add_subdirectory(src/app)
add_subdirectory(src/business)

# 是否构建单元测试（默认开启）。可通过 -DBUILD_TESTING=OFF 关闭。
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
	# 启用 CTest 测试框架
	enable_testing()
	# 添加 tests 子目录（内含各测试用例）
	add_subdirectory(tests)
endif()
