# CMakeList.txt: CMakeYZL 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

cmake_minimum_required (VERSION 3.8)
project("DemoYZL")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

link_directories(${CMAKE_SOURCE_DIR}/3dparty/libjpeg/lib)
link_directories(${CMAKE_SOURCE_DIR}/3dparty/libtiff/lib/)
link_directories(${CMAKE_SOURCE_DIR}/3dparty/zlib/bin/)


# 将源代码添加到此项目的可执行文件。
add_executable (${PROJECT_NAME} "CMakeYZL.cpp" "CMakeYZL.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        libjpeg
        tiffd
        zlibd
        zlibstaticd
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        libjpeg
        tiff
        zlib
        zlibstatic
    )
endif()