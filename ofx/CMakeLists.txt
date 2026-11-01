# MeatVideo.ofx (N4a / N4b / N4d): the OFX image-effect plug-in over libnvcore's C ABI (native/nvcore_c*.h), and the test
# host.  ofx/build_ofx.sh builds this and assembles MeatVideo.ofx.bundle.
#
#   cmake -S ofx -B ofx/build && cmake --build ofx/build -j 2
#   ofx/build_ofx.sh                      (-> ofx/build/MeatVideo.ofx.bundle)
#   ofx/build/minihost ofx/build/MeatVideo.ofx.bundle --check     (the plug-in structure, ofx/tools/minihost.cpp)
cmake_minimum_required(VERSION 3.21)
project(meatvideo_ofx LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# libnvcore.dylib (native/Makefile `lib`, the flags of the verified N1 ports), as qtui/CMakeLists.txt
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

# float32 rules of the native ports (the plug-in only moves buffers; it follows them anyway)
add_compile_options(-ffp-contract=off -fno-fast-math -Wall -Wextra)

# the plug-in: a Mach-O bundle exporting the three OFX entry points only
# (PyObj, the JSON value of the C++ UI, reads ClipParams and writes the window's request: qtui/src/core/pyobj.cpp)
set(QTUI_CORE ${CMAKE_CURRENT_SOURCE_DIR}/../qtui/src/core)
add_library(MeatVideo_ofx MODULE src/plugin.cpp src/params.cpp src/core.cpp src/session.cpp src/modal_wait.cpp
            ${QTUI_CORE}/pyobj.cpp)
target_include_directories(MeatVideo_ofx PRIVATE src vendor/openfx/include ${QTUI_CORE})
# libobjc: the host's Cocoa event loop while a button waits for the window (src/modal_wait.h); AppKit / Foundation are
# not linked (used only when the host has them loaded).  MV_MODAL_WAIT=OFF: the plain blocking wait (control builds)
option(MV_MODAL_WAIT "pump the host's Cocoa events while the window is open" ON)
target_compile_definitions(MeatVideo_ofx PRIVATE MV_MODAL_WAIT=$<BOOL:${MV_MODAL_WAIT}>)
target_link_libraries(MeatVideo_ofx PRIVATE nvcore objc)
set_target_properties(MeatVideo_ofx PROPERTIES
  OUTPUT_NAME MeatVideo PREFIX "" SUFFIX ".ofx"
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON
  BUILD_RPATH "@loader_path/../Frameworks;${NVCORE_DIR}"
  INSTALL_RPATH "@loader_path/../Frameworks")
target_link_options(MeatVideo_ofx PRIVATE
  "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exports.txt")

# the test host (it reads test frames and writes results itself; the plug-in never does)
# (not linked with libnvcore: the plug-in must load the copy inside its bundle)
add_executable(minihost tools/minihost.cpp)
target_include_directories(minihost PRIVATE vendor/openfx/include)

# the same host inside a running Cocoa application (the plug-in's waits on the main thread of NSApp): tools/cocoahost.mm
add_executable(cocoahost tools/cocoahost.mm)
target_include_directories(cocoahost PRIVATE vendor/openfx/include tools)
target_compile_options(cocoahost PRIVATE -fobjc-arc)
target_link_libraries(cocoahost PRIVATE "-framework AppKit")

# the file-access monitor injected into the test host (DYLD_INSERT_LIBRARIES; ofx/tools/fsguard.c)
add_library(fsguard SHARED tools/fsguard.c)
