cmake_minimum_required (VERSION 3.5)
project(tinyrenderer)

# 1. C++ 标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 寻找 OpenMP (MSVC 自带)
find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# 3. 包含头文件路径
include_directories(src
                    src/Test
                    src/Texture
                    src/Shader
                    src/Scene
                    src/ThreadPool
                    src/ObjLoader
)

# 4. 寻找所有源码并存入 SOURCES 变量，供内层 CMake 使用
file(GLOB_RECURSE SOURCES "src/*.h" "src/*.cpp")

# 5. 【核心修复】设置 OpenCV 路径
# 使用你刚刚找到的官方 build 路径，它包含适配 MSVC 的 vc16 目录
set(OpenCV_DIR "E:/test/opencv/build")
find_package(OpenCV REQUIRED)

# 6. 【核心修复】设置 TBB 路径
set(TBB_DIR "E:/test/oneapi-tbb-2022.3.0-win/oneapi-tbb-2022.3.0/lib/cmake/tbb")
find_package(TBB REQUIRED)

# 7. 进入子目录
add_subdirectory(src)