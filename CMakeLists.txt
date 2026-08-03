cmake_minimum_required(VERSION 3.16)

project(
    SiteHelper
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

# Third-party dependency
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

# Our libraries/applications
add_subdirectory(commandmenu)
add_subdirectory(geometry)
add_subdirectory(renderer2d)
add_subdirectory(sitehelper)