add_library(platform_event INTERFACE)

target_include_directories(platform_event
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(platform_event_sdl
    platform_event_sdl.c
)

target_include_directories(platform_event_sdl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(platform_event_sdl
    PUBLIC
        platform_event
    PRIVATE
        SDL3::SDL3
)

add_executable(test_platform_event_sdl
    test_platform_event_sdl.c
)

target_link_libraries(test_platform_event_sdl
    PRIVATE
        platform_event_sdl
        SDL3::SDL3
)

add_test(
    NAME platform_event_sdl_tests
    COMMAND test_platform_event_sdl
)
