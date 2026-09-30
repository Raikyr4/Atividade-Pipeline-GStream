cmake_minimum_required(VERSION 3.20)
project(
    PipelineMultimidia
    VERSION 1.0.0
    LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pipeline_multimidia src/principal.cpp)
if(MSVC)
    set(GSTREAMER_RAIZ
        "$ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}"
        CACHE PATH "Pasta do SDK GStreamer MSVC x64")
    find_path(GST_INCLUDE gst/gst.h PATHS "${GSTREAMER_RAIZ}/include/gstreamer-1.0" REQUIRED)
    find_path(GLIB_INCLUDE glib.h PATHS "${GSTREAMER_RAIZ}/include/glib-2.0" REQUIRED)
    find_path(GLIB_CONFIG glibconfig.h PATHS "${GSTREAMER_RAIZ}/lib/glib-2.0/include" REQUIRED)
    target_include_directories(pipeline_multimidia PRIVATE "${GST_INCLUDE}" "${GLIB_INCLUDE}"
                                                           "${GLIB_CONFIG}")
    foreach(BIBLIOTECA gstreamer-1.0 gstvideo-1.0 gstbase-1.0 gobject-2.0 glib-2.0)
        find_library(
            LIB_${BIBLIOTECA}
            NAMES ${BIBLIOTECA}
            PATHS "${GSTREAMER_RAIZ}/lib" REQUIRED)
        target_link_libraries(pipeline_multimidia PRIVATE "${LIB_${BIBLIOTECA}}")
    endforeach()
    target_compile_options(pipeline_multimidia PRIVATE /W4 /utf-8)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.18
                      gstreamer-video-1.0>=1.18)
    target_link_libraries(pipeline_multimidia PRIVATE PkgConfig::GSTREAMER)
    target_compile_options(pipeline_multimidia PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(
        Python3
        COMPONENTS Interpreter
        REQUIRED)
    add_test(NAME integracao
             COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/testes/verificar.py"
                     "$<TARGET_FILE:pipeline_multimidia>")
    set_tests_properties(integracao PROPERTIES TIMEOUT 60)
endif()
