cmake_minimum_required(VERSION 3.13)
project(Persistence)

find_package(PNG)
if(NOT PNG_FOUND)
  find_package(ZLIB) # zlib is needed by libPNG
  if(NOT ZLIB_FOUND)
        set(ZLIB_BUILD_EXAMPLES OFF)
        add_subdirectory(third_party/zlib-1.3.1)
        set(ZLIB_FOUND TRUE)
        set(PNG_BUILD_ZLIB ON)
        set_target_properties(${ZLIB_LIBRARIES} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIRS}")
        add_library(ZLIB::ZLIB ALIAS "${ZLIB_LIBRARIES}")
  endif()

  set(PNG_SHARED OFF)
  set(PNG_TESTS OFF)
  set(PNG_TOOLS OFF)
  set(SKIP_INSTALL_ALL 1)
  add_subdirectory(third_party/libpng-1.6.44)
  set_target_properties(${PNG_LIBRARIES} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PNG_INCLUDE_DIRS}")
  add_library(PNG::PNG ALIAS "${PNG_LIBRARIES}")
endif()

add_executable(reeb
    io_png.c io_png.h
    cmdLine.h
    draw_curve.cpp draw_curve.h
    fill_curve.cpp fill_curve.h
    levelLine.cpp levelLine.h
    lltree.cpp lltree.h
    reeb.cpp)

target_link_libraries(reeb PRIVATE PNG::PNG)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU)|(CLANG)")
  set_target_properties(reeb PROPERTIES COMPILE_FLAGS "-Wall -Wextra")
endif()

# UtilSaddles
add_executable(int_saddles UtilsSaddles/int_saddles.cpp
                           UtilsSaddles/saddle.cpp)
add_executable(delta_saddles UtilsSaddles/delta_saddles.cpp
                             UtilsSaddles/saddle.cpp)
