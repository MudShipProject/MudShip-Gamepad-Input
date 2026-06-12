cmake_minimum_required(VERSION 3.20)
project(GameInputBridge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(GameInputBridge SHARED GameInputBridge.cpp)
# 出力名は MS_GamepadBridge（Microsoft純正 GameInputBridge.dll との衝突回避）
set_target_properties(GameInputBridge PROPERTIES OUTPUT_NAME "MS_GamepadBridge")
target_compile_options(GameInputBridge PRIVATE /utf-8)

# --- GameInput SDK の場所（GameInput.h / GameInput.lib）---
# Windows SDK 10.0.26100 以降に同梱。例:
#   -DGAMEINPUT_INCLUDE_DIR="C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um"
#   -DGAMEINPUT_LIB="C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/GameInput.lib"
if(NOT DEFINED GAMEINPUT_INCLUDE_DIR OR NOT DEFINED GAMEINPUT_LIB)
	message(FATAL_ERROR
		"GAMEINPUT_INCLUDE_DIR と GAMEINPUT_LIB を指定してください "
		"（GameInput.h のフォルダ と GameInput.lib のフルパス）")
endif()

target_include_directories(GameInputBridge PRIVATE "${GAMEINPUT_INCLUDE_DIR}")
target_link_libraries(GameInputBridge PRIVATE "${GAMEINPUT_LIB}")
