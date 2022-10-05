# CMakeList.txt: ThreadPool 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("ThreadPool")

# 将源代码添加到此项目的可执行文件。
message(${PROJECT_SOURCE_DIR})
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)
add_executable (ThreadPool ${SRC})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ThreadPool PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
