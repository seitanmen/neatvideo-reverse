# qtui - the C++ Qt 6 UI (N3a) of the reproduction: the PySide6 UI repro/nv_ui_qt rebuilt in C++, all
# processing through libnvcore's C ABI (native/nvcore_c*.h).  No Python at run time.
#
#   cmake -S qtui -B qtui/build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build qtui/build -j 2
#   QT_QPA_PLATFORM=offscreen qtui/build/qtui_selftest            (Qt Test selftest)
#   qtui/build/MeatVideoQt.app/Contents/MacOS/MeatVideoQt [video]  (the UI)
cmake_minimum_required(VERSION 3.21)
project(nvqtui LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Test)

# libnvcore.dylib (native/Makefile `lib`, the same flags as the verified N1 ports)
set(NVCORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../native)
add_custom_command(OUTPUT ${NVCORE_DIR}/libnvcore.dylib
  COMMAND make -C ${NVCORE_DIR} lib
  WORKING_DIRECTORY ${NVCORE_DIR}
  COMMENT "make -C native lib")
add_custom_target(nvcore_build DEPENDS ${NVCORE_DIR}/libnvcore.dylib)
add_library(nvcore SHARED IMPORTED)
set_target_properties(nvcore PROPERTIES IMPORTED_LOCATION ${NVCORE_DIR}/libnvcore.dylib
  INTERFACE_INCLUDE_DIRECTORIES ${NVCORE_DIR})
add_dependencies(nvcore nvcore_build)

# float32 rules of the native ports (the UI does little arithmetic itself; it follows them anyway)
add_compile_options(-ffp-contract=off -fno-fast-math -Wall -Wextra)

file(GLOB NVUI_SRC CONFIGURE_DEPENDS src/core/*.cpp src/core/*.h src/widgets/*.cpp src/widgets/*.h
     src/app/*.cpp src/app/*.h)
add_library(nvui STATIC ${NVUI_SRC})
target_include_directories(nvui PUBLIC src src/core src/widgets src/app)
target_link_libraries(nvui PUBLIC Qt6::Widgets nvcore)
target_compile_definitions(nvui PUBLIC NV_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(MeatVideoQt MACOSX_BUNDLE src/main.cpp)
target_link_libraries(MeatVideoQt PRIVATE nvui)
set_target_properties(MeatVideoQt PROPERTIES
  BUILD_RPATH "${NVCORE_DIR}"
  MACOSX_BUNDLE_GUI_IDENTIFIER org.reproduction.meatvideo.qt
  MACOSX_BUNDLE_BUNDLE_NAME MeatVideoQt)

add_executable(qtui_selftest tests/selftest.cpp)
target_link_libraries(qtui_selftest PRIVATE nvui Qt6::Test)
set_target_properties(qtui_selftest PROPERTIES BUILD_RPATH "${NVCORE_DIR}")

# dump tool for the comparisons with the Python UI (qtui/tools/compare_*.py)
add_executable(qtui_dump tests/dump.cpp)
target_link_libraries(qtui_dump PRIVATE nvui)
set_target_properties(qtui_dump PROPERTIES BUILD_RPATH "${NVCORE_DIR}")

# command-line export (N3b): the same arguments as `python3 -m nv_ui_qt.export` (qtui/tools/compare_export.py)
add_executable(qtui_export tests/export_cli.cpp)
target_link_libraries(qtui_export PRIVATE nvui)
set_target_properties(qtui_export PROPERTIES BUILD_RPATH "${NVCORE_DIR}")

# OFX host mode test driver (N3c): nvqt::ofx_main + the steps of $NV_OFX_SCRIPT (qtui/tools/compare_ofx.py)
add_executable(qtui_ofx_drv tests/ofx_drv.cpp)
target_link_libraries(qtui_ofx_drv PRIVATE nvui)
set_target_properties(qtui_ofx_drv PROPERTIES BUILD_RPATH "${NVCORE_DIR}")

# the N3c comparison scenario (Preferences, Profile Viewer, Batch Profiler, Bit depth): qtui/tools/compare_n3c.py
add_executable(qtui_dump_n3c tests/dump_n3c.cpp)
target_link_libraries(qtui_dump_n3c PRIVATE nvui)
set_target_properties(qtui_dump_n3c PROPERTIES BUILD_RPATH "${NVCORE_DIR}")

# G1 (R/G/B equalizer) comparison driver: qtui/tools/g1_compare.py
add_executable(qtui_g1eq tests/g1_eq.cpp)
target_link_libraries(qtui_g1eq PRIVATE nvui)
set_target_properties(qtui_g1eq PROPERTIES BUILD_RPATH "${NVCORE_DIR}")
