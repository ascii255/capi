cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

option(CAPI_ENABLE_TESTING "Enable testing" OFF)
option(CAPI_STATIC_TESTING "Enable compile-time testing" OFF)
option(CAPI_BUILD_TESTING "Build testing binary" OFF)
if(CAPI_STATIC_TESTING OR CAPI_BUILD_TESTING)
  set(CAPI_ENABLE_TESTING ON CACHE BOOL "Enable testing" FORCE)
endif()

project(CAPI VERSION 1.0.0 LANGUAGES CXX)

add_library(capi INTERFACE)
add_library(CAPI::capi ALIAS capi)
target_sources(capi INTERFACE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES
    include/capi/unique_id.hpp
    include/capi/unique_res.hpp
)
target_compile_features(capi INTERFACE cxx_std_23)
target_compile_definitions(capi INTERFACE $<$<BOOL:${CAPI_ENABLE_TESTING}>:CAPI_TESTING>)
target_compile_definitions(capi INTERFACE $<$<BOOL:${CAPI_STATIC_TESTING}>:CAPI_STATIC_TESTING>)
target_include_directories(capi SYSTEM INTERFACE include include/single)

if(CAPI_BUILD_TESTING)
  enable_testing()
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
  add_executable(capi_testing main.cpp)
  target_compile_options(capi_testing PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
      /W4
      /WX
      /permissive-
    >
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wshadow
      -Wconversion
      -Wsign-conversion
      -Wnull-dereference
      -Wdouble-promotion
    >
  )
  target_link_libraries(capi_testing PRIVATE CAPI::capi)
  add_test(NAME unique_id COMMAND $<TARGET_FILE:capi_testing> unique_id)
  add_test(NAME unique_res COMMAND $<TARGET_FILE:capi_testing> unique_res)
endif()
