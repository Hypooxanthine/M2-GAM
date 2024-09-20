cmake_minimum_required(VERSION 3.8)

# ----- Project definition -----

project(TP)

# ----- Project directories -----

set(SOURCE_DIR src)
set(INCLUDE_DIR include)

# ----- Project files -----

# Header files
file(GLOB_RECURSE PROJECT_HEADERS   ${INCLUDE_DIR}/*.h)
set(HEADERS ${PROJECT_HEADERS})

# Source files
file(GLOB_RECURSE PROJECT_IMPL      ${SOURCE_DIR}/*.cpp)
set(SOURCES ${PROJECT_IMPL})

# ----- Binaries building -----

add_executable(TP                      ${PROJECT_IMPL} ${PROJECT_HEADERS})
target_include_directories(TP  PUBLIC  ${INCLUDE_DIR})
target_link_libraries(TP               Vroom)

# Compile options
if (MSVC)
    target_compile_options(TP PRIVATE /MP)
endif()

# ----- Specific settings -----

# Visual Studio specific settings
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # Grouping project files
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})
    # Set the debugger working directory
    set_property(TARGET TP PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:TP>)
endif()

# ----- Copying resource files -----

project(Resources NONE)

# Resource directories
set(ENGINE_RESOURCE_DIR ${CMAKE_SOURCE_DIR}/Resources)
set(TP_RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources)

# Output directory. For visual studio, we need to append the configuration name
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set(OUTPUT_DIR ${TP_BINARY_DIR}/$<CONFIG>/Resources)
else()
    set(OUTPUT_DIR ${TP_BINARY_DIR}/Resources)
endif()

add_custom_target(TPResources ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${ENGINE_RESOURCE_DIR} ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_RESOURCE_DIR} ${OUTPUT_DIR}
    COMMENT "Copying resource files to output directory"
)

# Make sure Resources is built before TP
add_dependencies(TP TPResources)
