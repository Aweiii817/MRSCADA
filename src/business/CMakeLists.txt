# business 业务模块 — 用户、配方、报表
# 静态库，依赖 core / simulation / common
add_library(business STATIC
    user/User.h
    user/UserManager.cpp
    user/UserManager.h
    recipe/Recipe.h
    recipe/RecipeManager.cpp
    recipe/RecipeManager.h
    recipe/RecipeExecutor.cpp
    recipe/RecipeExecutor.h
    recipe/ReportGenerator.cpp
    recipe/ReportGenerator.h
)

target_link_libraries(business PUBLIC
    core
    common
    simulation        # RecipeExecutor 需要 SimEngine
    Qt6::Core
    Qt6::Gui
    Qt6::Sql           # UserManager 需要 SQLite 数据库
    Qt6::PrintSupport  # ReportGenerator 需要 QPrinter
    nlohmann_json::nlohmann_json
)

target_include_directories(business PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
