# The E9 field-processing tools (INT-3) on a scratch copy of the int2 worktree (native/ qtui/ data/; the worktree
# itself is not touched):
#   make -C $SRC/native lib
#   cmake -S work/omo/wp_e9/tools/cpp_int3 -B $BUILD -DSRC_ROOT=$SRC -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos
#   cmake --build $BUILD -j 2
# e9_profile_fields (the UI's field profile of an interlaced clip) and qtui_export (the copy's export_cli.cpp).
cmake_minimum_required(VERSION 3.21)
project(e9tools_int3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)

set(ROOT ${SRC_ROOT})
if(NOT ROOT)
  message(FATAL_ERROR "-DSRC_ROOT=<copy of the int2 worktree (native/ qtui/ data/)> is required")
endif()
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

add_executable(e9_profile_fields e9_profile_fields.cpp)
target_link_libraries(e9_profile_fields PRIVATE nvui)
set_target_properties(e9_profile_fields PROPERTIES BUILD_RPATH "${NVCORE_DIR}")

add_executable(qtui_export ${QTUI}/tests/export_cli.cpp)
target_link_libraries(qtui_export PRIVATE nvui)
set_target_properties(qtui_export PROPERTIES BUILD_RPATH "${NVCORE_DIR}")
