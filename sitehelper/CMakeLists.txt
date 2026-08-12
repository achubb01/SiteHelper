add_library(sitehelper_core
    project/sitehelper_project.c

    app/appstate.c

    domain/domain_id.c

    editor/snap.c
    editor/opening_tool.c
    editor/opening_placement.c
    editor/opening_command.c

    wall/wall_generation.c
    wall/wall_lifecycle.c
    wall/build_structure.c
    wall/wall_position_generation.c
    wall/wall_noggins.c
    wall/wall_studs.c
    wall/wall_members.c
    wall/wall_openings.c
    wall/wall_render.c
    wall/wall_query.c
    wall/wall_selection.c
    wall/wall_editor.c
    wall/wall_snap.c

    gui/gui_layout.c
    gui/gui_render.c
    gui/gui_button.c
    gui/gui_toolbar.c

    grid/grid_render.c
    grid/grid_snap.c
)

target_include_directories(sitehelper_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/project
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/cli
        ${CMAKE_CURRENT_SOURCE_DIR}/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/editor
        ${CMAKE_CURRENT_SOURCE_DIR}/grid
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
        ${CMAKE_CURRENT_SOURCE_DIR}/model
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

add_executable(test_snap
    editor/tests/test_snap.c
)

target_link_libraries(test_snap
    PRIVATE
        sitehelper_core
)

add_test(
    NAME snap_tests
    COMMAND test_snap
)

add_executable(test_wall_snap
    wall/tests/test_wall_snap.c
)

target_link_libraries(test_wall_snap
    PRIVATE
        sitehelper_core
)

add_test(
    NAME wall_snap_tests
    COMMAND test_wall_snap
)

add_executable(test_gui_layout
    gui/tests/test_gui_layout.c
)

target_link_libraries(test_gui_layout
    PRIVATE
        sitehelper_core
)

add_test(
    NAME gui_layout_tests
    COMMAND test_gui_layout
)

add_executable(test_gui_button
    gui/tests/test_gui_button.c
)

target_link_libraries(test_gui_button
    PRIVATE
        sitehelper_core
)

add_test(
    NAME gui_button_tests
    COMMAND test_gui_button
)

add_executable(test_gui_toolbar
    gui/tests/test_gui_toolbar.c
)

target_link_libraries(test_gui_toolbar
    PRIVATE
        sitehelper_core
)

add_test(
    NAME gui_toolbar_tests
    COMMAND test_gui_toolbar
)

add_executable(test_opening_tool
    editor/tests/test_opening_tool.c
)

target_link_libraries(test_opening_tool
    PRIVATE
        sitehelper_core
)

add_test(
    NAME opening_tool_tests
    COMMAND test_opening_tool
)

add_executable(test_opening_placement
    editor/tests/test_opening_placement.c
)

target_link_libraries(test_opening_placement
    PRIVATE
        sitehelper_core
)

add_test(
    NAME opening_placement_tests
    COMMAND test_opening_placement
)

add_executable(test_opening_command
    editor/tests/test_opening_command.c
)

target_link_libraries(test_opening_command
    PRIVATE
        sitehelper_core
)

add_test(
    NAME opening_command_tests
    COMMAND test_opening_command
)

add_executable(test_domain_id
    domain/tests/test_domain_id.c
)

target_link_libraries(test_domain_id
    PRIVATE
        sitehelper_core
)

add_test(
    NAME domain_id_tests
    COMMAND test_domain_id
)

add_executable(test_build_structure
    wall/tests/test_build_structure.c
)

target_link_libraries(test_build_structure
    PRIVATE
        sitehelper_core
)

add_test(
    NAME build_structure_tests
    COMMAND test_build_structure
)

add_executable(test_appstate
    app/tests/test_appstate.c
)

target_link_libraries(test_appstate
    PRIVATE
        sitehelper_core
)

add_test(
    NAME appstate_tests
    COMMAND test_appstate
)

add_executable(test_sitehelper_project
    project/tests/test_sitehelper_project.c
)

target_link_libraries(test_sitehelper_project
    PRIVATE
        sitehelper_core
)

add_test(
    NAME sitehelper_project_tests
    COMMAND test_sitehelper_project
)

add_executable(sitehelper
    app/main.c
    cli/actions.c
    cli/appmenu.c
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