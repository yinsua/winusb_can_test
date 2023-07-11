cmake_minimum_required(VERSION 3.23)
project(winusb_can_test)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(IMGUI_DIR "C:/imgui_lib/imgui")

include_directories(${IMGUI_DIR})

# 关闭警告C4819
# warning C4819: 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失
add_compile_options(/wd4819)

file(GLOB SOURCE
        ${IMGUI_DIR}/*.cpp
        ${IMGUI_DIR}/*.h
        ${IMGUI_DIR}/backends/imgui_impl_glfw.*
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.*
        ${IMGUI_DIR}/backends/imgui_impl_win32.*)
# glfw
set(glfw3_DIR "C:\\Program Files (x86)\\GLFW\\lib\\cmake\\glfw3")
find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
link_libraries(${GLFW_LIBRARY_DIRS})

# opengl
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# glew
set(glew_DIR "C:\\Program Files (x86)\\glew\\lib\\cmake\\glew")
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

file(GLOB HW_SRC
        main.cpp
        mainwindows.cpp
        candle.c
        candle_ctrl_req.c
        gs_usb.c
        gsusb.c
        )
add_executable(winusb_can_test main.cpp ${SOURCE} ${HW_SRC})
target_link_libraries(winusb_can_test
        glfw
        ${OPENGL_LIBRARIES}
        GLEW::GLEW
        wsock32 ws2_32
        onecore
        SetupApi Ole32 winusb
        )