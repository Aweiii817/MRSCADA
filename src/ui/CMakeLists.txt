# ui 库 — 界面组件模块
add_library(ui STATIC
    MonitorWidget.cpp
    MonitorWidget.h
    DataAnalysisWidget.cpp
    DataAnalysisWidget.h
    DashboardWidget.cpp
    DashboardWidget.h
    GaugeWidget.cpp 
    GaugeWidget.h
    StatusIndicator.cpp 
    StatusIndicator.h
    LevelBar.cpp 
    LevelBar.h
    TagTableModel.cpp 
    TagTableModel.h
    DataTableWidget.cpp 
    DataTableWidget.h
    RealTimeTrend.cpp 
    RealTimeTrend.h
    HistoryTrend.cpp 
    HistoryTrend.h
    ProcessView.cpp 
    ProcessView.h
    process/ReactorItem.cpp 
    process/ReactorItem.h
    process/TankItem.cpp 
    process/TankItem.h
    process/PipeItem.cpp 
    process/PipeItem.h
    process/ValveItem.cpp 
    process/ValveItem.h
    process/PumpItem.cpp 
    process/PumpItem.h
    alarm/AlarmTableModel.cpp 
    alarm/AlarmTableModel.h
    alarm/AlarmListWidget.cpp 
    alarm/AlarmListWidget.h
    log/LogViewWidget.cpp 
    log/LogViewWidget.h
    recipe/RecipeEditorWidget.cpp 
    recipe/RecipeEditorWidget.h
    report/ReportWidget.cpp 
    report/ReportWidget.h
)

target_link_libraries(ui PUBLIC
    core
    common
    simulation
    business
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)

target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})