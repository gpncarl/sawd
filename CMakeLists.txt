cmake_minimum_required(VERSION 3.14)

set(CMAKE_VERBOSE_MAKEFILE ON)
include(cmake/prelude.cmake)

find_program(SCCACHE_PROGRAM sccache)
find_program(CCACHE_PROGRAM ccache)
if(SCCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
elseif(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

project(
    sawd
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    sawd_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    sawd_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(sawd_lib PUBLIC cxx_std_20)
# target_compile_options(sawd_lib PRIVATE -Wno-unused-command-line-argument)

find_package(fmt REQUIRED)
target_link_libraries(sawd_lib PRIVATE fmt::fmt)

find_package(stdexec CONFIG REQUIRED)
target_link_libraries(sawd_lib PRIVATE STDEXEC::stdexec)

# target_compile_definitions(sawd_lib PUBLIC UVW_BUILD_LIBS)
# find_package(uvw CONFIG REQUIRED)
# target_link_libraries(sawd_lib PRIVATE uvw::uvw)
find_package(libuv CONFIG REQUIRED)
target_link_libraries(sawd_lib PRIVATE $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(sawd_lib PUBLIC spdlog::spdlog)

# ---- Declare executable ----

add_executable(sawd_exe source/main.cpp)
add_executable(sawd::exe ALIAS sawd_exe)

set_property(TARGET sawd_exe PROPERTY OUTPUT_NAME sawd)

target_compile_features(sawd_exe PRIVATE cxx_std_20)

target_link_libraries(sawd_exe PRIVATE sawd_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT sawd_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of sawd"
  )
endif()

include(cmake/dev-mode.cmake)
