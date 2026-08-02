add_library(sitehelper_core
    sitehelper.c
    appstate.c
    wall_render.c
    wall_query.c
    wall_selection.c
    wall_editor.c
)

target_include_directories(sitehelper_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sitehelper_core
    PUBLIC
        renderer2d
)


add_executable(test_sitehelper
    test_sitehelper.c
)

target_link_libraries(test_sitehelper
    PRIVATE
        sitehelper_core
)

add_test(
    NAME sitehelper_tests
    COMMAND test_sitehelper
)


add_executable(test_wall_render
    test_wall_render.c
)

target_link_libraries(test_wall_render
    PRIVATE
        sitehelper_core
)

if(NOT MSVC)
    target_link_libraries(test_wall_render
        PRIVATE
            m
    )
endif()

add_test(
    NAME wall_render_tests
    COMMAND test_wall_render
)


add_executable(test_wall_query
    test_wall_query.c
)

target_link_libraries(test_wall_query
    PRIVATE
        sitehelper_core
)

add_test(
    NAME wall_query_tests
    COMMAND test_wall_query
)

add_executable(test_wall_selection
    test_wall_selection.c
)

target_link_libraries(test_wall_selection
    PRIVATE
        sitehelper_core
)

add_test(
    NAME wall_selection_tests
    COMMAND test_wall_selection
)

add_executable(test_wall_editor
    test_wall_editor.c
)

target_link_libraries(test_wall_editor
    PRIVATE
        sitehelper_core
)

add_test(
    NAME wall_editor_tests
    COMMAND test_wall_editor
)


add_executable(sitehelper
    main.c
    actions.c
    appmenu.c
)

target_link_libraries(sitehelper
    PRIVATE
        sitehelper_core
        commandmenu
)

add_executable(sitehelper_sdl
    sitehelper_sdl.c
)

target_link_libraries(sitehelper_sdl
    PRIVATE
        sitehelper_core
        renderer2d_sdl
)

if(NOT MSVC)
    target_link_libraries(sitehelper_sdl
        PRIVATE
            m
    )
endif()

if(WIN32)
    add_custom_command(
        TARGET sitehelper_sdl
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:sitehelper_sdl>
    )
endif()