cmake_minimum_required(VERSION 3.21)
project(BRSnowfall VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "BR Snowfall v0.1.0 currently supports Windows only")
endif()

include(FetchContent)
set(GLEW_USE_STATIC_LIBS TRUE)
find_package(GLEW REQUIRED)
set(FFGL_BUILD_EXAMPLE_PLUGINS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  ffgl
  GIT_REPOSITORY https://github.com/resolume/ffgl.git
  GIT_TAG 5b6e07840277df9ec35a228b9eb067636316350f
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ffgl)

add_library(BRSnowfall SHARED
  src/BRSnowfall.cpp
  src/BRSnowfall.h
  FFGLPlugin.def
)

target_compile_features(BRSnowfall PRIVATE cxx_std_17)
target_compile_definitions(BRSnowfall PRIVATE NOMINMAX)
target_link_libraries(BRSnowfall PRIVATE ffgl::sdk GLEW::GLEW opengl32)
target_include_directories(BRSnowfall PRIVATE ${ffgl_SOURCE_DIR}/source/lib)

set_target_properties(BRSnowfall PROPERTIES
  OUTPUT_NAME "BR Snowfall"
  PREFIX ""
  SUFFIX ".dll"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugin"
)

if(MSVC)
  target_compile_options(BRSnowfall PRIVATE /W4 /permissive- /utf-8)
  target_link_options(BRSnowfall PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/FFGLPlugin.def")
endif()

