add_library(sitehelper_model
    model/build_structure.c
    domain/domain_id.c
    model/build_settings.c
    model/wall_plan_segment.c
    model/wall_plan_transform.c
)

target_include_directories(sitehelper_model
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/model
)

if(NOT MSVC)
    target_link_libraries(sitehelper_model PRIVATE m)
endif()

add_library(sitehelper_wall
    wall/wall_plan.c
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
    project/project_settings.c
    project/room_separator.c
    project/sitehelper_project.c
    project/project_validation.c
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

add_library(sitehelper_persistence
    persistence/sitehelper_persistence.c
)

option(SITEHELPER_BUILD_TOPOLOGY
    "Build exact derived plan topology with portable 128-bit arithmetic" ON)
if(SITEHELPER_BUILD_TOPOLOGY)
    add_library(sitehelper_topology
        topology/plan_topology.c
        topology/plan_topology_query.c
        topology/project_topology.c
        topology/room_region.c
        topology/wall_junctions.c
    )
    if(MSVC)
        target_sources(sitehelper_topology PRIVATE topology/topology_numeric_msvc.c)
    else()
        target_sources(sitehelper_topology PRIVATE topology/topology_numeric.c)
    endif()
    target_include_directories(sitehelper_topology PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/topology)
    target_link_libraries(sitehelper_topology PUBLIC sitehelper_project PRIVATE sitehelper_wall)
endif()

target_include_directories(sitehelper_persistence
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/persistence
)

target_link_libraries(sitehelper_persistence
    PUBLIC
        sitehelper_project
    PRIVATE
        sitehelper_wall
)

add_library(sitehelper_command
    command/opening_command.c
    command/edit_opening_command.c
    command/wall_command.c
    command/delete_wall_command.c
    command/move_wall_endpoint_command.c
    command/room_location_command.c
    command/room_separator_command.c
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
    editor/editor_properties.c
    editor/editor_snap_state.c
    editor/snap.c
    editor/opening_tool.c
    editor/opening_placement.c
    editor/wall_tool.c
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
    gui/text_edit.c
    gui/length_parse.c
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
    app/app_input.c
    app/app_input_hud.c
    app/appstate.c
    app/app_view.c
)

target_include_directories(sitehelper_app_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(sitehelper_app_core
    PUBLIC
        sitehelper_render
        platform_event
        sitehelper_project
        sitehelper_editor
        sitehelper_command
    PRIVATE
        sitehelper_wall
)

add_library(sitehelper_test_support INTERFACE)

target_include_directories(sitehelper_test_support
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(sitehelper_test_support
    INTERFACE
        sitehelper_project
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

add_executable(test_wall_plan wall/tests/test_wall_plan.c)
target_link_libraries(test_wall_plan PRIVATE sitehelper_wall)
add_test(NAME wall_plan_tests COMMAND test_wall_plan)

add_executable(test_wall_plan_transform model/tests/test_wall_plan_transform.c)
target_link_libraries(test_wall_plan_transform PRIVATE sitehelper_model)
add_test(NAME wall_plan_transform_tests COMMAND test_wall_plan_transform)

add_executable(test_wall_generation_invariants
    wall/tests/test_wall_generation_invariants.c
)

target_link_libraries(test_wall_generation_invariants
    PRIVATE
        sitehelper_wall
        sitehelper_test_support
)

add_test(
    NAME wall_generation_invariant_tests
    COMMAND test_wall_generation_invariants
)

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

add_executable(test_wall_tool
    editor/tests/test_wall_tool.c
)

target_link_libraries(test_wall_tool
    PRIVATE
        sitehelper_editor
)

add_test(NAME wall_tool_tests COMMAND test_wall_tool)

add_executable(test_multiwall_editor
    editor/tests/test_multiwall_editor.c
)

target_link_libraries(test_multiwall_editor
    PRIVATE
        sitehelper_editor
        sitehelper_wall
)

add_test(NAME multiwall_editor_tests COMMAND test_multiwall_editor)

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

add_executable(test_edit_opening_command command/tests/test_edit_opening_command.c)
target_link_libraries(test_edit_opening_command PRIVATE sitehelper_command sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_edit_opening_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_edit_opening_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME edit_opening_command_tests COMMAND test_edit_opening_command)

add_executable(test_editor_properties editor/tests/test_editor_properties.c)
target_link_libraries(test_editor_properties PRIVATE sitehelper_editor sitehelper_test_support)
add_test(NAME editor_properties_tests COMMAND test_editor_properties)

add_executable(test_opening_geometry wall/tests/test_opening_geometry.c)
target_link_libraries(test_opening_geometry PRIVATE sitehelper_test_support)
add_test(NAME opening_geometry_tests COMMAND test_opening_geometry)

add_executable(test_opening_migration persistence/tests/test_opening_migration.c)
target_link_libraries(test_opening_migration PRIVATE sitehelper_persistence sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_opening_migration PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_opening_migration PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME opening_migration_tests COMMAND test_opening_migration)

add_executable(test_wall_command
    command/tests/test_wall_command.c
)

target_link_libraries(test_wall_command
    PRIVATE
        sitehelper_command
        sitehelper_wall
)

add_test(NAME wall_command_tests COMMAND test_wall_command)

add_executable(test_delete_wall_command command/tests/test_delete_wall_command.c)
target_link_libraries(test_delete_wall_command PRIVATE sitehelper_command sitehelper_test_support)
# Link-time allocation failures exercise transactions without production hooks.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_delete_wall_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_delete_wall_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME delete_wall_command_tests COMMAND test_delete_wall_command)

add_executable(test_move_wall_endpoint_command command/tests/test_move_wall_endpoint_command.c)
target_link_libraries(test_move_wall_endpoint_command PRIVATE
    sitehelper_command sitehelper_test_support sitehelper_persistence)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_move_wall_endpoint_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_move_wall_endpoint_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME move_wall_endpoint_command_tests COMMAND test_move_wall_endpoint_command)

add_executable(test_room_location_command command/tests/test_room_location_command.c)
target_link_libraries(test_room_location_command PRIVATE sitehelper_command sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_room_location_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_room_location_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME room_location_command_tests COMMAND test_room_location_command)

add_executable(test_room_separator_command command/tests/test_room_separator_command.c)
target_link_libraries(test_room_separator_command PRIVATE sitehelper_command sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_room_separator_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_room_separator_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME room_separator_command_tests COMMAND test_room_separator_command)

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

add_executable(test_app_view app/tests/test_app_view.c)
target_link_libraries(test_app_view PRIVATE sitehelper_app_core sitehelper_persistence)
add_test(NAME app_view_tests COMMAND test_app_view)

add_executable(test_viewport_input
    app/tests/test_viewport_input.c
    app/viewport_input.c
)

target_include_directories(test_viewport_input
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(test_viewport_input
    PRIVATE
        sitehelper_gui
)

add_test(NAME viewport_input_tests COMMAND test_viewport_input)

add_executable(test_sitehelper_project
    project/tests/test_sitehelper_project.c
)

target_link_libraries(test_sitehelper_project
    PRIVATE
        sitehelper_project
        sitehelper_wall
)

add_test(NAME sitehelper_project_tests COMMAND test_sitehelper_project)

add_executable(test_room_location project/tests/test_room_location.c)
target_link_libraries(test_room_location PRIVATE sitehelper_project sitehelper_test_support)
add_test(NAME room_location_tests COMMAND test_room_location)

add_executable(test_room_separator project/tests/test_room_separator.c)
target_link_libraries(test_room_separator PRIVATE sitehelper_project sitehelper_test_support)
add_test(NAME room_separator_tests COMMAND test_room_separator)

if(SITEHELPER_BUILD_TOPOLOGY)
    add_executable(test_plan_topology topology/tests/test_plan_topology.c)
    target_link_libraries(test_plan_topology PRIVATE sitehelper_topology sitehelper_test_support)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(test_plan_topology PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
        target_link_options(test_plan_topology PRIVATE
            "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
    endif()
    add_test(NAME plan_topology_tests COMMAND test_plan_topology)
    add_executable(test_room_region topology/tests/test_room_region.c)
    target_link_libraries(test_room_region PRIVATE sitehelper_topology sitehelper_test_support
        sitehelper_editor sitehelper_persistence)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(test_room_region PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
        target_link_options(test_room_region PRIVATE
            "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
    endif()
    add_test(NAME room_region_tests COMMAND test_room_region)
    add_executable(test_wall_junctions topology/tests/test_wall_junctions.c)
    target_link_libraries(test_wall_junctions PRIVATE sitehelper_topology
        sitehelper_test_support sitehelper_command sitehelper_persistence)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(test_wall_junctions PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
        target_link_options(test_wall_junctions PRIVATE
            "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
    endif()
    add_test(NAME wall_junction_tests COMMAND test_wall_junctions)
endif()

add_executable(test_storeys project/tests/test_storeys.c)
target_link_libraries(test_storeys PRIVATE sitehelper_test_support sitehelper_app_core sitehelper_persistence)
if(SITEHELPER_BUILD_TOPOLOGY)
    target_compile_definitions(test_storeys PRIVATE SITEHELPER_TEST_TOPOLOGY)
    target_link_libraries(test_storeys PRIVATE sitehelper_topology)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_storeys PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_storeys PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME storey_tests COMMAND test_storeys)

add_executable(test_project_settings project/tests/test_project_settings.c)
target_link_libraries(test_project_settings PRIVATE sitehelper_test_support sitehelper_app_core sitehelper_persistence)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_project_settings PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_project_settings PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME project_settings_tests COMMAND test_project_settings)

add_executable(test_project_validation project/tests/test_project_validation.c)
target_link_libraries(test_project_validation PRIVATE sitehelper_project)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_project_validation PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_project_validation PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME project_validation_tests COMMAND test_project_validation)

add_executable(test_sitehelper_persistence
    persistence/tests/test_sitehelper_persistence.c
)

target_link_libraries(test_sitehelper_persistence
    PRIVATE
        sitehelper_persistence
        sitehelper_wall
        sitehelper_test_support
)

add_test(
    NAME sitehelper_persistence_tests
    COMMAND test_sitehelper_persistence
)

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

add_executable(command_invariant_tests
    command/tests/test_command_invariants.c
)

target_link_libraries(command_invariant_tests
    PRIVATE
        sitehelper_command
        sitehelper_wall
        sitehelper_test_support
)

add_test(NAME command_invariant_tests COMMAND command_invariant_tests)

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
        sitehelper_gui
)

add_executable(sitehelper_sdl
    app/sitehelper_sdl.c
    app/viewport_input.c
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

add_executable(test_text_input gui/tests/test_text_input.c)
target_link_libraries(test_text_input PRIVATE sitehelper_gui)
add_test(NAME text_input_tests COMMAND test_text_input)

add_executable(test_app_input app/tests/test_app_input.c)
target_link_libraries(test_app_input PRIVATE sitehelper_app_core)
add_test(NAME app_input_tests COMMAND test_app_input)

add_executable(test_sdl_input app/tests/test_sdl_input.c app/viewport_input.c)
target_link_libraries(test_sdl_input PRIVATE sitehelper_app_core renderer2d_sdl platform_event_sdl SDL3::SDL3)
add_test(NAME sdl_input_tests COMMAND test_sdl_input)

add_executable(test_cli_measurements cli/tests/test_cli_measurements.c cli/actions.c)
target_include_directories(test_cli_measurements PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cli)
target_link_libraries(test_cli_measurements PRIVATE sitehelper_app_core sitehelper_gui)
add_test(NAME cli_measurement_tests COMMAND test_cli_measurements)
