find_package(GTest REQUIRED)

include(GoogleTest)

add_executable(test_ucs
    test_main.cpp
    test_tag_manager.cpp
    test_ring_buffer.cpp
    test_sim_reactor.cpp
)

target_link_libraries(test_ucs PRIVATE
    core          
    common
    simulation
    GTest::GTest  
    GTest::Main   
    Qt6::Core
    Qt6::Test
)

target_include_directories(test_ucs PRIVATE ${CMAKE_SOURCE_DIR}/src)

gtest_discover_tests(test_ucs)