cmake_minimum_required(VERSION 3.15)
project(custom_vehicles_server VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SA-MP plugins DEBEN ser 32-bit. Configurar con: cmake -A Win32 ..
set(SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/samp-plugin-sdk")

if(NOT EXISTS "${SDK_DIR}/plugincommon.h")
    message(FATAL_ERROR
        "SA-MP Plugin SDK no encontrado en deps/samp-plugin-sdk\n"
        "Ejecuta desde C:/samp-custom-vehicles:\n"
        "  git init\n"
        "  git submodule add https://github.com/Zeex/samp-plugin-sdk server/deps/samp-plugin-sdk"
    )
endif()

add_library(custom_vehicles SHARED
    src/main.cpp
    src/model_manager.cpp
    ${SDK_DIR}/amxplugin.cpp
)

if(MSVC)
    set_target_properties(custom_vehicles PROPERTIES
        LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/src/custom_vehicles.def\""
    )
endif()

target_include_directories(custom_vehicles PRIVATE
    src
    ${SDK_DIR}
    ${SDK_DIR}/amx
)

set_target_properties(custom_vehicles PROPERTIES
    OUTPUT_NAME "custom_vehicles"
    PREFIX ""
)

if(WIN32)
    set_target_properties(custom_vehicles PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(custom_vehicles PROPERTIES SUFFIX ".so")
endif()
