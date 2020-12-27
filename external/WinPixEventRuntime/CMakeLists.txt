cmake_minimum_required(VERSION 3.13)
add_library (WinPixEventRuntime INTERFACE)
target_include_directories(WinPixEventRuntime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_link_libraries(WinPixEventRuntime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib)