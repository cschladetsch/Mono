cmake_minimum_required(VERSION 3.27)
set(PROJ_TESTS ${PROJ}_tests)

# TODO
#add_executable(${PROJ} compare_memory_pool)
add_executable(${PROJ_TESTS} tests.cpp)

#target_include_directories(${PROJ} PUBLIC ${CMAKE_SOURCE_DIR})
#target_include_directories(${PROJ} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJ_TESTS} PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(${PROJ_TESTS} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
