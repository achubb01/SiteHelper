add_library(sitehelper_core
    sitehelper.c
    app/appstate.c

    wall/wall_render.c
    wall/wall_query.c
    wall/wall_selection.c
    wall/wall_editor.c

    grid/grid_render.c
    grid/grid_snap.c
)

target_include_directories(sitehelper_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/grid
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/wall
)

target_link_libraries(sitehelper_core
    PUBLIC
        renderer2d
)

if(NOT MSVC)
    target_link_libraries(sitehelper_core
        PUBLIC
            m
    )
endif()

add_executable(test_sitehelper
    tests/test_sitehelper.c
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
    wall/tests/test_wall_render.c
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
    wall/tests/test_wall_query.c
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
    wall/tests/test_wall_selection.c
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
    wall/tests/test_wall_editor.c
)

target_link_libraries(test_wall_editor
    PRIVATE
        sitehelper_core
)

add_test(
    NAME wall_editor_tests
    COMMAND test_wall_editor
)

add_executable(test_grid_render
    grid/tests/test_grid_render.c
)

target_link_libraries(test_grid_render
    PRIVATE
        sitehelper_core
)

add_test(
    NAME grid_render_tests
    COMMAND test_grid_render
)

add_executable(test_grid_snap
    grid/tests/test_grid_snap.c
)

target_link_libraries(test_grid_snap
    PRIVATE
        sitehelper_core
)

add_test(
    NAME grid_snap_tests
    COMMAND test_grid_snap
)


add_executable(sitehelper
    app/main.c
    ui/actions.c
    ui/appmenu.c
)

target_link_libraries(sitehelper
    PRIVATE
        sitehelper_core
        commandmenu
)

add_executable(sitehelper_sdl
    app/sitehelper_sdl.c
)

target_link_libraries(sitehelper_sdl
    PRIVATE
        sitehelper_core
        renderer2d_sdl
)

if(WIN32)
    add_custom_command(
        TARGET sitehelper_sdl
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:sitehelper_sdl>
    )
endif()