cmake_minimum_required(VERSION 3.20)

project(StabledDonkey VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SCRIPTHOOK_RDR2_SDK "${CMAKE_CURRENT_SOURCE_DIR}/sdk/ScriptHookRDR2" CACHE PATH
    "Path to the locally installed ScriptHookRDR2 SDK")

if(NOT EXISTS "${SCRIPTHOOK_RDR2_SDK}/inc/natives.h" OR
   NOT EXISTS "${SCRIPTHOOK_RDR2_SDK}/lib/ScriptHookRDR2.lib")
    message(FATAL_ERROR
        "ScriptHookRDR2 SDK not found. Set SCRIPTHOOK_RDR2_SDK to its extracted directory.")
endif()

add_library(StabledDonkey SHARED
    src/main.cpp
    src/mount/MountAdapter.cpp
)

target_include_directories(StabledDonkey PRIVATE
    "${SCRIPTHOOK_RDR2_SDK}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mount"
)

target_link_directories(StabledDonkey PRIVATE
    "${SCRIPTHOOK_RDR2_SDK}/lib"
)

target_link_libraries(StabledDonkey PRIVATE
    ScriptHookRDR2
)

set_target_properties(StabledDonkey PROPERTIES
    OUTPUT_NAME "StabledDonkey"
    SUFFIX ".asi"
)
