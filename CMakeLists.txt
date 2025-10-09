cmake_minimum_required(VERSION 3.10)
project(foolhex)

# 添加FLTK子模块
add_subdirectory(thirdparty/fltk)

# 创建可执行文件
add_executable(${PROJECT_NAME} src/main.cpp src/kmp.cpp src/LargeFile.cpp src/HexTable.cpp src/HexEditorWindow.cpp)

# 链接FLTK库
target_link_libraries(${PROJECT_NAME} PRIVATE fltk)

# Windows系统需要额外链接的库
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        wsock32
        comctl32
    )
    # 设置Windows的C++ ABI兼容性
    add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)
endif()
