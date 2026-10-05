cmake_minimum_required(VERSION 3.16)
project(IDMTrialReset C)

set(CMAKE_C_STANDARD 11)

# Windows-only project
if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows only.")
endif()

add_executable(IDMTrialReset WIN32
    src/main.c
    src/core.c
    src/resource.rc
)

target_include_directories(IDMTrialReset PRIVATE src)

target_link_libraries(IDMTrialReset
    comctl32
    wininet
    shlwapi
)

# Set subsystem to Windows (no console window)
set_target_properties(IDMTrialReset PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "IDM Trial Reset"
)
