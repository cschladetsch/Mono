cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.84 REQUIRED)

set(PROJ monotonic)
project(${PROJ})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Boost)
find_package(Boost REQUIRED COMPONENTS chrono filesystem system)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Assuming your header files are located in the "include" directory
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

# Filter out system header files
foreach(HEADER_FILE ${HEADER_FILES})
    get_filename_component(FILE_NAME ${HEADER_FILE} NAME)
    if(NOT "${FILE_NAME}" MATCHES "^(assert|windows|other_system_header).h$")
        list(APPEND FILTERED_HEADER_FILES ${HEADER_FILE})
    endif()
endforeach()

# Create a filter named "Header Files" in Visual Studio
source_group("Header Files" FILES ${FILTERED_HEADER_FILES})

# Add your header files to your target
# TODO
#add_executable(${PROJ} ${FILTERED_HEADER_FILES} <other_source_files>)
source_group("include" FILE ${HEADER_FILES})

add_subdirectory(test)

