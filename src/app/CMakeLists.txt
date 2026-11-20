# add_executable����һ����ִ�г���
add_executable(UCS
    main.cpp
    MainWindow.cpp
    MainWindow.h
    LoginDialog.cpp
    LoginDialog.h
)

target_link_libraries(UCS PRIVATE
    core         
    common
    simulation
    comm
    ui
    business
    Qt6::Core     
    Qt6::Widgets  
)

target_include_directories(UCS PRIVATE ${CMAKE_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------
# 部署 SQLite 数据库驱动插件及其依赖
# UserManager 使用 SQLite 存储（通过 Qt6::Sql 的 QSQLITE 驱动）。
# vcpkg 的 applocal 部署只会复制直接链接的 Qt 库（如 Qt6Sqld.dll），
# 而 QSQLITE 驱动是运行时动态加载的插件（qsqlited.dll），其依赖的
# sqlite3.dll 也不会被自动复制，导致运行时出现 "Driver not loaded"。
# 这里在构建后手动复制 sqlite3.dll 与驱动插件到可执行文件目录。
# ---------------------------------------------------------------------------
add_custom_command(TARGET UCS POST_BUILD
    # 复制 SQLite 依赖库（QSQLITE 插件的动态链接依赖）
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:Qt6::Sql>/sqlite3.dll"
        "$<TARGET_FILE_DIR:UCS>/sqlite3.dll"
    # 创建 sqldrivers 插件子目录
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:UCS>/sqldrivers"
    # 复制 QSQLITE 驱动插件（Debug/Release 文件名后缀不同）
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:Qt6::Sql>/../Qt6/plugins/sqldrivers/$<$<CONFIG:Debug>:qsqlited.dll>$<$<CONFIG:Release>:qsqlite.dll>"
        "$<TARGET_FILE_DIR:UCS>/sqldrivers/"
)