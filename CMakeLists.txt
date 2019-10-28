cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(advisor DESCRIPTION "Advisor" VERSION 1.0.1 LANGUAGES CXX)
file(COPY res/launch.vs.json DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/.vs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE STRING "")
set(CMAKE_CXX_EXTENSIONS OFF CACHE STRING "")

set(CMAKE_CXX_FLAGS "/arch:AVX2 /fp:fast /permissive- /EHsc /utf-8")
set(CMAKE_CXX_FLAGS_DEBUG "/nologo /Od /Oi- /Ob0 /Gy- /GS /ZI /await")
set(CMAKE_CXX_FLAGS_RELEASE "/nologo /O1 /Oi /GS- /GL /await /await:heapelide")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/nologo /debug:fastlink /incremental /editandcontinue")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/nologo /debug:none /opt:ref /opt:icf /LTCG:incremental")
set(CMAKE_RC_FLAGS "/nologo /c65001")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>" CACHE STRING "")

set(PROJECT_VENDOR "Alexej Harm")
configure_file(res/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/version.h LF)

file(GLOB_RECURSE headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp src/*.manifest src/*.rc)

add_executable(${PROJECT_NAME} ${headers} ${sources})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/common.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE windowsapp)

if(CMAKE_BUILD_TYPE MATCHES Release)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
endif()
