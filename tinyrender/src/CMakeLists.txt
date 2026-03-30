cmake_minimum_required (VERSION 3.5)
project(MYRENDER)
# 【核心：强制指定生成器】必须放在 project() 之前！
# "Visual Studio 18 2026" 是 VS 2026 的官方 CMake 生成器名称
set(CMAKE_GENERATOR "Visual Studio 18 2026")

# 使用file(Glob) 找所有文件并赋给sources
# 下面是你原来的代码...
file(GLOB_RECURSE ADDITIONAL_SOURCES "*.h" "*.hpp" "*.cpp")

list(APPEND SOURCES ${ADDITIONAL_SOURCES})


add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb)

# SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/PROFILE")