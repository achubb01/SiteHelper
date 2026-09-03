add_library(sitehelper_model
    domain/domain_id.c
)

target_include_directories(sitehelper_model
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/model
)

add_library(sitehelper_wall
    wall/wall_generation.c
    wall/wall_lifecycle.c
    wall/build_structure.c
    wall/wall_position_generation.c
    wall/wall_noggins.c
    wall/wall_studs.c
    wall/wall_members.c
    wall/wall_openings.c
    wall/wall_query.c
    wall/wall_selection.c
)

target_include_directories(sitehelper_wall
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/wall
)

target_link_libraries(sitehelper_wall
    PUBLIC
        sitehelper_model
)

add_library(sitehelper_project
    project/sitehelper_project.c
)

target_include_directories(sitehelper_project
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/project
)

target_link_libraries(sitehelper_project
    PUBLIC
        sitehelper_model
    PRIVATE
        sitehelper_wall
)

add_library(sitehelper_command
    command/opening_command.c
    command/sitehelper_command.c
    command/command_history.c
)

target_include_directories(sitehelper_command
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/command
)

target_link_libraries(sitehelper_command
    PUBLIC
        sitehelper_project
    PRIVATE
        sitehelper_wall
)

add_library(sitehelper_grid
    grid/grid_snap.c
)

target_include_directories(sitehelper_grid
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/grid
)

target_link_libraries(sitehelper_grid
    PUBLIC
        geometry
)

if(NOT MSVC)
    target_link_libraries(sitehelper_grid
        PRIVATE
            m
    )
endif()

add_library(sitehelper_editor
    editor/sitehelper_editor.c
    editor/editor_selection.c
    editor/editor_snap_state.c
    editor/snap.c
    editor/opening_tool.c
    editor/opening_placement.c
    editor/editor_action.c

    wall/wall_snap.c
)

target_include_directories(sitehelper_editor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/editor
)

target_link_libraries(sitehelper_editor
    PUBLIC
        sitehelper_model
        sitehelper_wall
        sitehelper_command
        geometry
    PRIVATE
        sitehelper_grid
)

add_library(sitehelper_gui
    gui/gui_layout.c
    gui/gui_button.c
    gui/gui_toolbar.c
)

target_include_directories(sitehelper_gui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

target_link_libraries(sitehelper_gui
    PUBLIC
        geometry
)

add_library(sitehelper_render
    wall/wall_render.c
    grid/grid_render.c
    gui/gui_render.c
)

target_include_directories(sitehelper_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/wall
        ${CMAKE_CURRENT_SOURCE_DIR}/grid
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

target_link_libraries(sitehelper_render
    PUBLIC
        renderer2d
        sitehelper_wall
        sitehelper_gui
        geometry
)

if(NOT MSVC)
    target_link_libraries(sitehelper_render
        PRIVATE
            m
    )
endif()

add_library(sitehelper_app_core
    app/appstate.c
)

target_include_directories(sitehelper_app_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(sitehelper_app_core
    PUBLIC
        sitehelper_project
        sitehelper_editor
        sitehelper_command
    PRIVATE
        sitehelper_wall
)

add_executable(test_sitehelper
    tests/test_sitehelper.c
)

target_link_libraries(test_sitehelper
    PRIVATE
        sitehelper_wall
)

add_test(NAME sitehelper_tests COMMAND test_sitehelper)

add_executable(test_wall_render
    wall/tests/test_wall_render.c
)

target_link_libraries(test_wall_render
    PRIVATE
        sitehelper_render
)

add_test(NAME wall_render_tests COMMAND test_wall_render)

add_executable(test_wall_query
    wall/tests/test_wall_query.c
)

target_link_libraries(test_wall_query
    PRIVATE
        sitehelper_wall
)

add_test(NAME wall_query_tests COMMAND test_wall_query)

add_executable(test_wall_selection
    wall/tests/test_wall_selection.c
)

target_link_libraries(test_wall_selection
    PRIVATE
        sitehelper_wall
)

add_test(NAME wall_selection_tests COMMAND test_wall_selection)

add_executable(test_wall_opening_validation
    wall/tests/test_wall_opening_validation.c
)

target_link_libraries(test_wall_opening_validation
    PRIVATE
        sitehelper_wall
)

add_test(
    NAME wall_opening_validation_tests
    COMMAND test_wall_opening_validation
)

add_executable(test_grid_render
    grid/tests/test_grid_render.c
)

target_link_libraries(test_grid_render
    PRIVATE
        sitehelper_render
)

add_test(NAME grid_render_tests COMMAND test_grid_render)

add_executable(test_grid_snap
    grid/tests/test_grid_snap.c
)

target_link_libraries(test_grid_snap
    PRIVATE
        sitehelper_grid
)

add_test(NAME grid_snap_tests COMMAND test_grid_snap)

add_executable(test_snap
    editor/tests/test_snap.c
)

target_link_libraries(test_snap
    PRIVATE
        sitehelper_editor
)

add_test(NAME snap_tests COMMAND test_snap)

add_executable(test_wall_snap
    wall/tests/test_wall_snap.c
)

target_link_libraries(test_wall_snap
    PRIVATE
        sitehelper_editor
)

add_test(NAME wall_snap_tests COMMAND test_wall_snap)

add_executable(test_gui_layout
    gui/tests/test_gui_layout.c
)

target_link_libraries(test_gui_layout
    PRIVATE
        sitehelper_gui
)

add_test(NAME gui_layout_tests COMMAND test_gui_layout)

add_executable(test_gui_button
    gui/tests/test_gui_button.c
)

target_link_libraries(test_gui_button
    PRIVATE
        sitehelper_gui
)

add_test(NAME gui_button_tests COMMAND test_gui_button)

add_executable(test_gui_toolbar
    gui/tests/test_gui_toolbar.c
)

target_link_libraries(test_gui_toolbar
    PRIVATE
        sitehelper_gui
)

add_test(NAME gui_toolbar_tests COMMAND test_gui_toolbar)

add_executable(test_opening_tool
    editor/tests/test_opening_tool.c
)

target_link_libraries(test_opening_tool
    PRIVATE
        sitehelper_editor
)

add_test(NAME opening_tool_tests COMMAND test_opening_tool)

add_executable(test_opening_placement
    editor/tests/test_opening_placement.c
)

target_link_libraries(test_opening_placement
    PRIVATE
        sitehelper_editor
)

add_test(NAME opening_placement_tests COMMAND test_opening_placement)

add_executable(test_opening_command
    command/tests/test_opening_command.c
)

target_link_libraries(test_opening_command
    PRIVATE
        sitehelper_command
        sitehelper_wall
)

add_test(NAME opening_command_tests COMMAND test_opening_command)

add_executable(test_domain_id
    domain/tests/test_domain_id.c
)

target_link_libraries(test_domain_id
    PRIVATE
        sitehelper_model
)

add_test(NAME domain_id_tests COMMAND test_domain_id)

add_executable(test_build_structure
    wall/tests/test_build_structure.c
)

target_link_libraries(test_build_structure
    PRIVATE
        sitehelper_wall
)

add_test(NAME build_structure_tests COMMAND test_build_structure)

add_executable(test_appstate
    app/tests/test_appstate.c
)

target_link_libraries(test_appstate
    PRIVATE
        sitehelper_app_core
)

add_test(NAME appstate_tests COMMAND test_appstate)

add_executable(test_sitehelper_project
    project/tests/test_sitehelper_project.c
)

target_link_libraries(test_sitehelper_project
    PRIVATE
        sitehelper_project
        sitehelper_wall
)

add_test(NAME sitehelper_project_tests COMMAND test_sitehelper_project)

add_executable(test_sitehelper_editor
    editor/tests/test_sitehelper_editor.c
)

target_link_libraries(test_sitehelper_editor
    PRIVATE
        sitehelper_editor
)

add_test(NAME sitehelper_editor_tests COMMAND test_sitehelper_editor)

add_executable(test_editor_selection
    editor/tests/test_editor_selection.c
)

target_link_libraries(test_editor_selection
    PRIVATE
        sitehelper_editor
)

add_test(NAME editor_selection_tests COMMAND test_editor_selection)

add_executable(test_editor_snap_state
    editor/tests/test_editor_snap_state.c
)

target_link_libraries(test_editor_snap_state
    PRIVATE
        sitehelper_editor
)

add_test(NAME editor_snap_state_tests COMMAND test_editor_snap_state)

add_executable(sitehelper_command_tests
    command/tests/test_sitehelper_command.c
)

target_link_libraries(sitehelper_command_tests
    PRIVATE
        sitehelper_command
        sitehelper_wall
)

add_test(
    NAME sitehelper_command_tests
    COMMAND sitehelper_command_tests
)

add_executable(command_history_tests
    command/tests/test_command_history.c
)

target_link_libraries(command_history_tests
    PRIVATE
        sitehelper_command
        sitehelper_wall
)

add_test(NAME command_history_tests COMMAND command_history_tests)

add_executable(sitehelper
    app/main.c
    cli/actions.c
    cli/appmenu.c
)

target_include_directories(sitehelper
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cli
)

target_link_libraries(sitehelper
    PRIVATE
        sitehelper_app_core
        sitehelper_wall
        commandmenu
)

add_executable(sitehelper_sdl
    app/sitehelper_sdl.c
)

target_link_libraries(sitehelper_sdl
    PRIVATE
        sitehelper_app_core
        sitehelper_render
        renderer2d_sdl
        platform_event_sdl
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
