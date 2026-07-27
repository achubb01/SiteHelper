add_library(renderer2d
    renderer2d.c
)

target_include_directories(renderer2d
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(renderer2d
    PUBLIC
        geometry
)


add_executable(test_renderer2d
    test_renderer2d.c
)

target_link_libraries(test_renderer2d
    PRIVATE
        renderer2d
)

if(NOT MSVC)
    target_link_libraries(test_renderer2d
        PRIVATE
            m
    )
endif()

add_test(
    NAME renderer2d_tests
    COMMAND test_renderer2d
)


add_library(renderer2d_sdl
    renderer2d_sdl.c
)

target_include_directories(renderer2d_sdl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(renderer2d_sdl
    PUBLIC
        renderer2d

    PRIVATE
        SDL3::SDL3
)


add_executable(demo_renderer2d_sdl
    demo_renderer2d_sdl.c
)

target_link_libraries(demo_renderer2d_sdl
    PRIVATE
        renderer2d_sdl
)