cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)

project(Ruminoid_RenderCore)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include(conan.cmake)
conan_check(REQUIRED)

add_library(ruminoid_rendercore SHARED ruminoid_rendercore.cpp "ruminoid_rendercore.h")

set(CONAN_OPTIONS "")

conan_cmake_run(REQUIRES
                    "libass/0.14.0@charliejiang/stable"
                BASIC_SETUP CMAKE_TARGETS
                OPTIONS ${CONAN_OPTIONS}
                ENV "CONAN_CMAKE_GENERATOR=Ninja"
                BUILD missing)

if (WIN32)
    target_compile_definitions(ruminoid_rendercore PUBLIC "-D_WIN32_WINNT=0x0601" "-DNOMINMAX")
    target_compile_options(ruminoid_rendercore PUBLIC "/utf-8")
endif()
target_compile_definitions(ruminoid_rendercore PUBLIC "-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
target_link_libraries(ruminoid_rendercore
                        CONAN_PKG::libass
                        )
