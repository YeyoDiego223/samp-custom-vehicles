cmake_minimum_required(VERSION 3.15)
project(custom_vehicles_client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(custom_vehicles_client SHARED
    src/main.cpp
    src/model_loader.cpp
)

target_include_directories(custom_vehicles_client PRIVATE src)

# Salida como .asi (DLL renombrada que carga el ASI Loader)
set_target_properties(custom_vehicles_client PROPERTIES
    OUTPUT_NAME "custom_vehicles"
    PREFIX      ""
    SUFFIX      ".asi"
)

if(MSVC)
    # Deshabilitar incrementalidad para evitar thunks que rompen los address casts
    target_link_options(custom_vehicles_client PRIVATE /INCREMENTAL:NO)
    # Runtime estatico para no depender de MSVC redistributable en GTA SA
    set_property(TARGET custom_vehicles_client PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
