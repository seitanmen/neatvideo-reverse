# The E9 tools on a private build of the C++ UI's sources (qtui/src at the checkout, built here so that the
# shared qtui/build_* directories are not touched):
#   cmake -S work/omo/wp_e9/tools/cpp -B work/omo/wp_e9/build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos
#   cmake --build work/omo/wp_e9/build -j 2
# e9_profile (the UI's default profile of a clip) and qtui_export (qtui/tests/export_cli.cpp, unchanged).
# libnvcore.dylib is the checkout's native/libnvcore.dylib (make -C native lib), used as is.
cmake_minimum_required(VERSION 3.21)
project(e9tools LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)

get_filename_component(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.. ABSOLUTE)
set(QTUI ${ROOT}/qtui)
set(NVCORE_DIR ${ROOT}/native)
add_library(nvcore SHARED IMPORTED)
set_target_properties(nvcore PROPERTIES IMPORTED_LOCATION ${NVCORE_DIR}/libnvcore.dylib
  INTERFACE_INCLUDE_DIRECTORIES ${NVCORE_DIR})

# the flags of qtui/CMakeLists.txt
add_compile_options(-ffp-contract=off -fno-fast-math -Wall -Wextra)
file(GLOB NVUI_SRC ${QTUI}/src/core/*.cpp ${QTUI}/src/core/*.h ${QTUI}/src/widgets/*.cpp ${QTUI}/src/widgets/*.h
     ${QTUI}/src/app/*.cpp ${QTUI}/src/app/*.h)
add_library(nvui STATIC ${NVUI_SRC})
target_include_directories(nvui PUBLIC ${QTUI}/src ${QTUI}/src/core ${QTUI}/src/widgets ${QTUI}/src/app)
target_link_libraries(nvui PUBLIC Qt6::Widgets nvcore)
target_compile_definitions(nvui PUBLIC NV_SOURCE_ROOT="${ROOT}")

add_executable(e9_profile e9_profile.cpp)
target_link_libraries(e9_profile PRIVATE nvui)
set_target_properties(e9_profile PROPERTIES BUILD_RPATH "${NVCORE_DIR}")

add_executable(qtui_export ${QTUI}/tests/export_cli.cpp)
target_link_libraries(qtui_export PRIVATE nvui)
set_target_properties(qtui_export PROPERTIES BUILD_RPATH "${NVCORE_DIR}")
