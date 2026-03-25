cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(CAPI VERSION 1.0.5 LANGUAGES CXX)

add_library(capi INTERFACE)
add_library(CAPI::capi ALIAS capi)
target_sources(capi INTERFACE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES
    include/capi/unique_flgd.hpp
    include/capi/unique_id.hpp
    include/capi/unique_res.hpp
    include/capi/unique_sys.hpp
)
target_compile_features(capi INTERFACE cxx_std_23)
target_include_directories(capi SYSTEM INTERFACE include include/single)

if(PROJECT_IS_TOP_LEVEL)
  add_executable(capi_testing main.cpp)
  target_compile_features(capi_testing PRIVATE cxx_std_23)
  target_include_directories(capi_testing PRIVATE include)
  target_compile_definitions(capi_testing PRIVATE CAPI_TESTING)
  target_compile_options(capi_testing PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /permissive->
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
      -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion>
  )
  set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
  include(CTest)
  add_test(NAME unique_flgd COMMAND $<TARGET_FILE:capi_testing> unique_flgd)
  add_test(NAME unique_id COMMAND $<TARGET_FILE:capi_testing> unique_id)
  add_test(NAME unique_res COMMAND $<TARGET_FILE:capi_testing> unique_res)
  add_test(NAME unique_sys COMMAND $<TARGET_FILE:capi_testing> unique_sys)
  include(Coverage)
  add_coverage(capi_testing capi)
endif()
