# CMakeLists.txt - AgenticAIOnWord Word Add-in
# Builds a 32-bit COM DLL for Microsoft Word 2010

cmake_minimum_required(VERSION 3.20)

# Project definition
project(AgenticAIOnWord 
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    # Use Unicode
    add_definitions(-D_UNICODE -DUNICODE)
    add_definitions(-D_WINDOWS -D_USRDLL -D_WINDLL)
    
    # ATL definitions
    add_definitions(-D_ATL_APARTMENT_THREADED)
    add_definitions(-D_ATL_NO_AUTOMATIC_NAMESPACE)
    add_definitions(-D_ATL_CSTRING_EXPLICIT_CONSTRUCTORS)
    add_definitions(-DATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW)
endif()

# Source files
set(SOURCES
    src/pch.cpp
    src/dllmain.cpp
    src/AgenticAIOnWord.cpp
    src/Connect.cpp
    src/TaskPaneControl.cpp
    AgenticAIOnWord_i.c
)

# Header files
set(HEADERS
    include/pch.h
    include/framework.h
    include/targetver.h
    include/Resource.h
    include/dllmain.h
    include/Connect.h
    include/TaskPaneControl.h
)

# Create shared library (DLL)
add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
    ${HEADERS}
    res/AgenticAIOnWord.rc
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries for COM/ATL
target_link_libraries(${PROJECT_NAME} PRIVATE
    kernel32
    user32
    gdi32
    advapi32
    shell32
    ole32
    oleaut32
    uuid
    comsuppw
)

# Module definition file
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/res/AgenticAIOnWord.def\""
    )
endif()

# Output settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "AgenticAIOnWord"
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Print configuration info
message(STATUS "")
message(STATUS "=== AgenticAIOnWord Build Configuration ===")
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}")
message(STATUS "")
