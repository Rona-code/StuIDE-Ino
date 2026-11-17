cmake_minimum_required(VERSION 3.16)

option(TARGET_ARM64 "Compilation croisée Linux ARM64 (clang + sysroot)" OFF)

if (TARGET_ARM64)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)

    set(SYSROOT $ENV{HOME}/sysroot-aarch64)
    set(GCC_DIR ${SYSROOT}/usr/lib/gcc/aarch64-redhat-linux/11)

    set(CMAKE_SYSROOT ${SYSROOT})
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER_TARGET aarch64-linux-gnu)
    set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-gnu)

    set(CMAKE_C_FLAGS_INIT   "--gcc-install-dir=${GCC_DIR}")
    set(CMAKE_CXX_FLAGS_INIT "--gcc-install-dir=${GCC_DIR}")
    set(CMAKE_EXE_LINKER_FLAGS_INIT    "-fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")

    set(CMAKE_FIND_ROOT_PATH ${SYSROOT})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

project(StuIDE-Ino
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------
# Version appli (unique)
# -------------------------
set(APP_VERSION "1.0.0")
set(APP_AUTHOR "Rona")
set(APP_SUMMARY "A lightweight C++ IDE for Arduino")
string(REPLACE "." "," APP_VERSION_COMMA "${APP_VERSION}")
add_definitions(-DAPP_VERSION="${APP_VERSION}")

# ------------------------- 
# Génération du fichier RC 
# ------------------------- 
if (WIN32) 
    string(REPLACE "\\" "/" CMAKE_SOURCE_DIR_FIXED "${CMAKE_SOURCE_DIR}")
    configure_file( 
        ${CMAKE_SOURCE_DIR}/app.rc.in 
        ${CMAKE_BINARY_DIR}/app.rc 
        @ONLY 
    ) 
    # Copie l'icône dans le build dir pour que le .rc la trouve
    configure_file(
        ${CMAKE_SOURCE_DIR}/resources/Icone.ico
        ${CMAKE_BINARY_DIR}/resources/Icone.ico
        COPYONLY
    )
endif()

# -------------------------
# Qt Widgets (statique)
# -------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# -------------------------
# Sources / Headers
# -------------------------
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/src/commun/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/ui/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "${CMAKE_SOURCE_DIR}/src/commun/*.h"
    "${CMAKE_SOURCE_DIR}/src/core/*.h"
    "${CMAKE_SOURCE_DIR}/src/ui/*.h"
)

# -------------------------
# Exécutable
# -------------------------
qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    ${SOURCES}
    ${HEADERS}
)

# -------------------------
# Copie Asset
# -------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

# -------------------------
# Ressources Qt (Windows + Linux)
# -------------------------
qt_add_resources(${PROJECT_NAME} "app_resources"
    PREFIX "/"
    FILES
        resources/Icone.png
        #resources/style/menu.qss
        resources/translations/stuide-ino_fr.qm
        resources/translations/stuide-ino_en.qm
        resources/translations/stuide-ino_ja.qm
)

# -------------------------
# Ressources Windows (.rc)
# -------------------------
if (WIN32)
    # Force le compilateur de ressources
    set_source_files_properties(${CMAKE_BINARY_DIR}/app.rc
        PROPERTIES
            LANGUAGE RC
    )
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
endif()

# -------------------------
# Includes
# -------------------------
target_include_directories(${PROJECT_NAME}
    PRIVATE
        
)

# -------------------------
# Link
# -------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        
)

# -------------------------
# Link Windows
# -------------------------
if (WIN32)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
    )
endif()

# -------------------------
# Link Linux
# -------------------------
if (UNIX AND NOT APPLE)
    #find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} 
        PRIVATE 
    #        ${X11_LIBRARIES}
    )
endif()