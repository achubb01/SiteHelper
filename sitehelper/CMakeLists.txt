add_library(sitehelper_cad_ir cad/cad_ir.c)
target_include_directories(sitehelper_cad_ir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cad)

add_executable(test_cad_ir cad/tests/test_cad_ir.c)
target_link_libraries(test_cad_ir PRIVATE sitehelper_cad_ir)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_cad_ir PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_cad_ir PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME cad_ir_tests COMMAND test_cad_ir)

add_library(sitehelper_cad_dxf_ascii cad/dxf_ascii.c)
target_include_directories(sitehelper_cad_dxf_ascii PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cad)
target_link_libraries(sitehelper_cad_dxf_ascii PUBLIC sitehelper_cad_ir)

add_executable(test_dxf_ascii cad/tests/test_dxf_ascii.c)
target_link_libraries(test_dxf_ascii PRIVATE sitehelper_cad_dxf_ascii)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_dxf_ascii PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_dxf_ascii PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME dxf_ascii_tests COMMAND test_dxf_ascii)

add_library(sitehelper_cad_plan_mapping cad/cad_plan_mapping.c)
target_include_directories(sitehelper_cad_plan_mapping
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/cad
        ${CMAKE_CURRENT_SOURCE_DIR}/model
)
target_link_libraries(sitehelper_cad_plan_mapping PUBLIC sitehelper_cad_ir)

add_executable(test_cad_plan_mapping cad/tests/test_cad_plan_mapping.c)
target_link_libraries(test_cad_plan_mapping PRIVATE
    sitehelper_cad_plan_mapping
    sitehelper_cad_dxf_ascii
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_cad_plan_mapping PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_cad_plan_mapping PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME cad_plan_mapping_tests COMMAND test_cad_plan_mapping)

add_library(sitehelper_cad_export_ir cad/cad_export_ir.c)
target_include_directories(sitehelper_cad_export_ir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cad)

add_library(sitehelper_cad_dxf_ascii_export cad/dxf_ascii_export.c)
target_include_directories(sitehelper_cad_dxf_ascii_export PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cad)
target_link_libraries(sitehelper_cad_dxf_ascii_export PUBLIC sitehelper_cad_export_ir)

add_executable(test_cad_export_ir cad/tests/test_cad_export_ir.c)
target_link_libraries(test_cad_export_ir PRIVATE
    sitehelper_cad_export_ir
    sitehelper_cad_dxf_ascii_export
    sitehelper_cad_dxf_ascii
    sitehelper_cad_plan_mapping
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_cad_export_ir PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_cad_export_ir PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME cad_export_ir_tests COMMAND test_cad_export_ir)

add_library(sitehelper_optimisation optimisation/linear_cut_optimizer.c)
target_include_directories(sitehelper_optimisation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/optimisation)

add_executable(test_linear_cut_optimizer optimisation/tests/test_linear_cut_optimizer.c)
target_link_libraries(test_linear_cut_optimizer PRIVATE sitehelper_optimisation)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_linear_cut_optimizer PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_linear_cut_optimizer PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME linear_cut_optimizer_tests COMMAND test_linear_cut_optimizer)

# Priority 26C numeric-contract prototype. This executable deliberately does
# not expose a production roof library/API; it freezes the arithmetic decision
# before the authoritative roof model is introduced in later priorities.
add_executable(test_roof_numeric_contract roof/tests/test_roof_numeric_contract.c)
if(NOT MSVC)
    target_link_libraries(test_roof_numeric_contract PRIVATE m)
endif()
add_test(NAME roof_numeric_contract_tests COMMAND test_roof_numeric_contract)

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

add_library(sitehelper_document document/document.c document/document_plan_query.c document/plan_dimension_geometry.c)
target_include_directories(sitehelper_document PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/document)
target_link_libraries(sitehelper_document PUBLIC sitehelper_model)

add_executable(test_document document/tests/test_document.c)
target_link_libraries(test_document PRIVATE sitehelper_document)
add_test(NAME document_tests COMMAND test_document)

# Priority 26D roof-intent/geometry prototype. This target remains isolated from
# SiteHelperProject/persistence/editor ownership until compound fixtures prove
# the source abstraction in 26E.
add_library(sitehelper_roof_geometry_prototype roof/roof_geometry_prototype.c)
target_include_directories(sitehelper_roof_geometry_prototype PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/roof)
target_link_libraries(sitehelper_roof_geometry_prototype PUBLIC sitehelper_model)

add_executable(test_roof_geometry_prototype roof/tests/test_roof_geometry_prototype.c)
target_link_libraries(test_roof_geometry_prototype PRIVATE sitehelper_roof_geometry_prototype)
add_test(NAME roof_geometry_prototype_tests COMMAND test_roof_geometry_prototype)

# Priority 26E1 compound/intersection prototype. Composition remains explicit
# source authority while valley/ridge clipping stays transient derived geometry.
add_library(sitehelper_roof_compound_prototype roof/roof_compound_prototype.c)
target_include_directories(sitehelper_roof_compound_prototype PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/roof)
target_link_libraries(sitehelper_roof_compound_prototype PUBLIC sitehelper_roof_geometry_prototype)

add_executable(test_roof_compound_prototype roof/tests/test_roof_compound_prototype.c)
target_link_libraries(test_roof_compound_prototype PRIVATE sitehelper_roof_compound_prototype)
add_test(NAME roof_compound_prototype_tests COMMAND test_roof_compound_prototype)

# Priority 26E2 termination-semantics prototype. The source carries primitive
# end/station intent; generated hips/cut geometry remain transient.
add_executable(test_roof_termination_prototype roof/tests/test_roof_termination_prototype.c)
target_link_libraries(test_roof_termination_prototype PRIVATE sitehelper_roof_compound_prototype)
add_test(NAME roof_termination_prototype_tests COMMAND test_roof_termination_prototype)

add_executable(test_roof_multilevel_composition_prototype
    roof/tests/test_roof_multilevel_composition_prototype.c)
target_link_libraries(test_roof_multilevel_composition_prototype
    PRIVATE sitehelper_roof_compound_prototype)
add_test(NAME roof_multilevel_composition_prototype_tests
    COMMAND test_roof_multilevel_composition_prototype)

# Priority 26G1 production roof authority. The authoritative source model is
# production; exact derived topology still delegates to the proven prototype
# kernel until its rational-boundary representation is promoted separately.
add_library(sitehelper_roof roof/roof.c roof/roof_plan_query.c)
target_include_directories(sitehelper_roof PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/roof)
target_link_libraries(sitehelper_roof PUBLIC sitehelper_model PRIVATE sitehelper_roof_compound_prototype)

add_executable(test_roof_domain roof/tests/test_roof_domain.c)
target_link_libraries(test_roof_domain PRIVATE sitehelper_roof sitehelper_roof_compound_prototype)
add_test(NAME roof_domain_tests COMMAND test_roof_domain)

add_executable(test_roof_plan_query roof/tests/test_roof_plan_query.c)
target_link_libraries(test_roof_plan_query PRIVATE sitehelper_roof)
add_test(NAME roof_plan_query_tests COMMAND test_roof_plan_query)

add_library(sitehelper_roof_structural_layout_prototype
    roof/roof_structural_layout_prototype.c)
target_include_directories(sitehelper_roof_structural_layout_prototype
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/roof)
target_link_libraries(sitehelper_roof_structural_layout_prototype
    PUBLIC sitehelper_roof_geometry_prototype)

add_executable(test_roof_structural_layout_prototype
    roof/tests/test_roof_structural_layout_prototype.c)
target_link_libraries(test_roof_structural_layout_prototype
    PRIVATE sitehelper_roof_structural_layout_prototype)
add_test(NAME roof_structural_layout_prototype_tests
    COMMAND test_roof_structural_layout_prototype)

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

add_library(sitehelper_surface surface/wall_surface.c)
target_include_directories(sitehelper_surface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/surface)
target_link_libraries(sitehelper_surface PUBLIC sitehelper_model PRIVATE sitehelper_wall)

add_executable(test_wall_surface surface/tests/test_wall_surface.c)
target_link_libraries(test_wall_surface PRIVATE sitehelper_surface sitehelper_wall)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_wall_surface PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_wall_surface PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME wall_surface_tests COMMAND test_wall_surface)

add_library(sitehelper_slab slab/slab.c)
target_include_directories(sitehelper_slab PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/slab)
target_link_libraries(sitehelper_slab PUBLIC sitehelper_model)
if(NOT MSVC)
    target_link_libraries(sitehelper_slab PRIVATE m)
endif()

add_library(sitehelper_slab_plan_query slab_plan/slab_plan_query.c)
target_include_directories(sitehelper_slab_plan_query PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/slab_plan)
target_link_libraries(sitehelper_slab_plan_query PUBLIC sitehelper_slab)
if(NOT MSVC)
    target_link_libraries(sitehelper_slab_plan_query PRIVATE m)
endif()

add_library(sitehelper_project
    project/project_cad_references.c
    project/project_roofs.c
    project/project_slabs.c
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
        sitehelper_slab
        sitehelper_roof
        sitehelper_document
        sitehelper_cad_plan_mapping
    PRIVATE
        sitehelper_wall
)

add_executable(test_cad_reference_ownership cad/tests/test_cad_reference_ownership.c)
target_link_libraries(test_cad_reference_ownership PRIVATE
    sitehelper_project
    sitehelper_cad_dxf_ascii
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_cad_reference_ownership PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_cad_reference_ownership PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME cad_reference_ownership_tests COMMAND test_cad_reference_ownership)

add_library(sitehelper_cad_plan_export cad/cad_plan_export.c)
target_include_directories(sitehelper_cad_plan_export
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/cad
        ${CMAKE_CURRENT_SOURCE_DIR}/project
)
target_link_libraries(sitehelper_cad_plan_export
    PUBLIC
        sitehelper_cad_export_ir
        sitehelper_project
)

add_executable(test_cad_plan_export cad/tests/test_cad_plan_export.c)
target_link_libraries(test_cad_plan_export PRIVATE
    sitehelper_cad_plan_export
    sitehelper_cad_dxf_ascii_export
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_cad_plan_export PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_cad_plan_export PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME cad_plan_export_tests COMMAND test_cad_plan_export)

add_library(sitehelper_takeoff takeoff/framing_takeoff.c)
target_include_directories(sitehelper_takeoff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/takeoff)
target_link_libraries(sitehelper_takeoff PUBLIC sitehelper_project)

add_executable(test_framing_takeoff takeoff/tests/test_framing_takeoff.c)
target_link_libraries(test_framing_takeoff PRIVATE sitehelper_takeoff sitehelper_wall)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_framing_takeoff PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_framing_takeoff PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME framing_takeoff_tests COMMAND test_framing_takeoff)

add_library(sitehelper_cutlist cutlist/cut_list.c)
target_include_directories(sitehelper_cutlist PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cutlist)
target_link_libraries(sitehelper_cutlist PUBLIC sitehelper_takeoff)

add_executable(test_cut_list cutlist/tests/test_cut_list.c)
target_link_libraries(test_cut_list PRIVATE sitehelper_cutlist sitehelper_wall)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_cut_list PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_cut_list PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME cut_list_tests COMMAND test_cut_list)

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
    command/create_slab_command.c
    command/delete_slab_command.c
    command/slab_feature_command.c
    command/edit_slab_command.c
    command/move_slab_vertex_command.c
    command/create_roof_command.c
    command/delete_roof_command.c
    command/roof_source_edit_command.c
    command/plan_note_command.c
    command/plan_dimension_command.c
    command/plan_symbol_command.c
    command/plan_callout_command.c
    command/plan_revision_cloud_command.c
    command/document_revision_command.c
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
    editor/editor_context.c
    editor/editor_selection.c
    editor/editor_properties.c
    editor/editor_snap_state.c
    editor/snap.c
    editor/plan_snap.c
    editor/opening_tool.c
    editor/opening_placement.c
    editor/wall_tool.c
    editor/measurement_tool.c
    editor/plan_dimension_tool.c
    editor/plan_callout_tool.c
    editor/plan_direction_symbol_tool.c
    editor/plan_revision_cloud_tool.c
    editor/slab_tool.c
    editor/slab_feature_tool.c
    editor/slab_geometry_tool.c
    editor/plan_position_conversion.c
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
        sitehelper_roof
        geometry
    PRIVATE
        sitehelper_grid
        sitehelper_slab_plan_query
)

if(SITEHELPER_BUILD_TOPOLOGY)
    target_link_libraries(sitehelper_editor PRIVATE sitehelper_topology)
    target_compile_definitions(sitehelper_editor PRIVATE SITEHELPER_HAS_TOPOLOGY)
endif()

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

add_library(sitehelper_slab_plan_render slab_plan/slab_plan_render.c)
target_include_directories(sitehelper_slab_plan_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/slab_plan)
target_link_libraries(sitehelper_slab_plan_render PUBLIC renderer2d sitehelper_slab_plan_query)

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
    app/app_properties_panel.c
    app/appstate.c
    app/app_view.c
    app/app_workspace.c
    app/presentation_composition.c
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
        sitehelper_slab_plan_render
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

add_executable(test_slab_editor_properties editor/tests/test_slab_editor_properties.c)
target_link_libraries(test_slab_editor_properties PRIVATE sitehelper_editor sitehelper_test_support)
add_test(NAME slab_editor_properties_tests COMMAND test_slab_editor_properties)

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

add_executable(test_slab_command command/tests/test_slab_command.c)
target_link_libraries(test_slab_command PRIVATE sitehelper_command sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME slab_command_tests COMMAND test_slab_command)

add_executable(test_roof_command command/tests/test_roof_command.c)
target_link_libraries(test_roof_command PRIVATE sitehelper_command sitehelper_test_support)
add_test(NAME roof_command_tests COMMAND test_roof_command)

add_executable(test_roof_source_edit_command command/tests/test_roof_source_edit_command.c)
target_link_libraries(test_roof_source_edit_command PRIVATE sitehelper_command sitehelper_test_support)
add_test(NAME roof_source_edit_command_tests COMMAND test_roof_source_edit_command)

add_executable(test_slab_feature_insert slab/tests/test_slab_feature_insert.c)
target_link_libraries(test_slab_feature_insert PRIVATE sitehelper_slab sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_feature_insert PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_feature_insert PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME slab_feature_insert_tests COMMAND test_slab_feature_insert)

add_executable(test_slab_property_command command/tests/test_slab_property_command.c)
target_link_libraries(test_slab_property_command PRIVATE sitehelper_command sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_property_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_property_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME slab_property_command_tests COMMAND test_slab_property_command)

add_executable(test_move_slab_vertex_command command/tests/test_move_slab_vertex_command.c)
target_link_libraries(test_move_slab_vertex_command PRIVATE sitehelper_command sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_move_slab_vertex_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_move_slab_vertex_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME move_slab_vertex_command_tests COMMAND test_move_slab_vertex_command)

add_executable(test_slab_feature_command command/tests/test_slab_feature_command.c)
target_link_libraries(test_slab_feature_command PRIVATE sitehelper_command sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_feature_command PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_feature_command PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME slab_feature_command_tests COMMAND test_slab_feature_command)

add_executable(test_slab_tool editor/tests/test_slab_tool.c)
target_link_libraries(test_slab_tool PRIVATE sitehelper_app_core sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_tool PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_tool PRIVATE "LINKER:--wrap=realloc")
endif()
add_test(NAME slab_tool_lifecycle_tests COMMAND test_slab_tool)

add_executable(test_slab_feature_delete editor/tests/test_slab_feature_delete.c)
target_link_libraries(test_slab_feature_delete PRIVATE sitehelper_app_core sitehelper_test_support)
add_test(NAME slab_feature_delete_routing_tests COMMAND test_slab_feature_delete)

add_executable(test_slab_feature_tools editor/tests/test_slab_feature_tools.c)
target_link_libraries(test_slab_feature_tools PRIVATE sitehelper_app_core sitehelper_test_support)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_feature_tools PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_feature_tools PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()
add_test(NAME slab_feature_tool_tests COMMAND test_slab_feature_tools)

add_executable(test_slab_geometry_tool editor/tests/test_slab_geometry_tool.c)
target_link_libraries(test_slab_geometry_tool PRIVATE sitehelper_app_core sitehelper_test_support)
add_test(NAME slab_geometry_tool_tests COMMAND test_slab_geometry_tool)

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

add_executable(test_app_workspace app/tests/test_app_workspace.c)
target_link_libraries(test_app_workspace PRIVATE sitehelper_app_core)
add_test(NAME app_workspace_tests COMMAND test_app_workspace)

add_executable(test_workspace_presentation app/tests/test_workspace_presentation.c)
target_link_libraries(test_workspace_presentation PRIVATE sitehelper_app_core)
add_test(NAME workspace_presentation_tests COMMAND test_workspace_presentation)

add_executable(test_presentation_composition app/tests/test_presentation_composition.c)
target_link_libraries(test_presentation_composition PRIVATE sitehelper_app_core)
add_test(NAME presentation_composition_tests COMMAND test_presentation_composition)

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

add_executable(test_project_document project/tests/test_project_document.c)
target_link_libraries(test_project_document PRIVATE sitehelper_project sitehelper_persistence)
add_test(NAME project_document_tests COMMAND test_project_document)

add_executable(test_project_dimensions project/tests/test_project_dimensions.c)
target_link_libraries(test_project_dimensions PRIVATE sitehelper_project sitehelper_persistence sitehelper_wall)
add_test(NAME project_dimension_tests COMMAND test_project_dimensions)

add_executable(test_project_symbols project/tests/test_project_symbols.c)
target_link_libraries(test_project_symbols PRIVATE sitehelper_project sitehelper_persistence)
add_test(NAME project_symbol_tests COMMAND test_project_symbols)

add_executable(test_project_callouts project/tests/test_project_callouts.c)
target_link_libraries(test_project_callouts PRIVATE sitehelper_project sitehelper_persistence)
add_test(NAME project_callout_tests COMMAND test_project_callouts)

add_executable(test_project_roofs project/tests/test_project_roofs.c)
target_link_libraries(test_project_roofs PRIVATE sitehelper_project sitehelper_persistence sitehelper_roof)
add_test(NAME project_roof_tests COMMAND test_project_roofs)

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

add_executable(test_editor_context
    editor/tests/test_editor_context.c
)

target_link_libraries(test_editor_context
    PRIVATE
        sitehelper_editor
)

add_test(NAME editor_context_tests COMMAND test_editor_context)

add_executable(test_workspace_selection
    editor/tests/test_workspace_selection.c
)
target_link_libraries(test_workspace_selection PRIVATE sitehelper_editor)
add_test(NAME workspace_selection_tests COMMAND test_workspace_selection)

add_executable(test_editor_selection
    editor/tests/test_editor_selection.c
)

target_link_libraries(test_editor_selection
    PRIVATE
        sitehelper_editor
)

add_test(NAME editor_selection_tests COMMAND test_editor_selection)

add_executable(test_roof_editor_integration
    editor/tests/test_roof_editor_integration.c
)
target_link_libraries(test_roof_editor_integration PRIVATE sitehelper_editor sitehelper_project)
add_test(NAME roof_editor_integration_tests COMMAND test_roof_editor_integration)

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

add_executable(test_plan_note_command command/tests/test_plan_note_command.c)
target_link_libraries(test_plan_note_command PRIVATE sitehelper_command)
add_test(NAME plan_note_command_tests COMMAND test_plan_note_command)

add_executable(test_plan_note_editor editor/tests/test_plan_note_editor.c)
target_link_libraries(test_plan_note_editor PRIVATE sitehelper_editor)
add_test(NAME plan_note_editor_tests COMMAND test_plan_note_editor)

add_executable(test_plan_dimension_command command/tests/test_plan_dimension_command.c)
target_link_libraries(test_plan_dimension_command PRIVATE sitehelper_command)
add_test(NAME plan_dimension_command_tests COMMAND test_plan_dimension_command)

add_executable(test_plan_symbol_command command/tests/test_plan_symbol_command.c)
target_link_libraries(test_plan_symbol_command PRIVATE sitehelper_command)
add_test(NAME plan_symbol_command_tests COMMAND test_plan_symbol_command)

add_executable(test_plan_callout_command command/tests/test_plan_callout_command.c)
target_link_libraries(test_plan_callout_command PRIVATE sitehelper_command)
add_test(NAME plan_callout_command_tests COMMAND test_plan_callout_command)

add_executable(test_plan_revision_cloud_command command/tests/test_plan_revision_cloud_command.c)
target_link_libraries(test_plan_revision_cloud_command PRIVATE sitehelper_command)
add_test(NAME plan_revision_cloud_command_tests COMMAND test_plan_revision_cloud_command)

add_executable(test_document_revision_command command/tests/test_document_revision_command.c)
target_link_libraries(test_document_revision_command PRIVATE sitehelper_command)
add_test(NAME document_revision_command_tests COMMAND test_document_revision_command)

add_executable(test_plan_dimension_editor editor/tests/test_plan_dimension_editor.c)
target_link_libraries(test_plan_dimension_editor PRIVATE sitehelper_editor)
add_test(NAME plan_dimension_editor_tests COMMAND test_plan_dimension_editor)

add_executable(test_plan_symbol_editor editor/tests/test_plan_symbol_editor.c)
target_link_libraries(test_plan_symbol_editor PRIVATE sitehelper_editor)
add_test(NAME plan_symbol_editor_tests COMMAND test_plan_symbol_editor)

add_executable(test_plan_direction_symbol_tool editor/tests/test_plan_direction_symbol_tool.c)
target_link_libraries(test_plan_direction_symbol_tool PRIVATE sitehelper_editor)
add_test(NAME plan_direction_symbol_tool_tests COMMAND test_plan_direction_symbol_tool)

add_executable(test_plan_callout_editor editor/tests/test_plan_callout_editor.c)
target_link_libraries(test_plan_callout_editor PRIVATE sitehelper_editor)
add_test(NAME plan_callout_editor_tests COMMAND test_plan_callout_editor)

add_executable(test_plan_callout_tool editor/tests/test_plan_callout_tool.c)
target_link_libraries(test_plan_callout_tool PRIVATE sitehelper_editor)
add_test(NAME plan_callout_tool_tests COMMAND test_plan_callout_tool)

add_executable(test_plan_revision_cloud_editor editor/tests/test_plan_revision_cloud_editor.c)
target_link_libraries(test_plan_revision_cloud_editor PRIVATE sitehelper_editor)
add_test(NAME plan_revision_cloud_editor_tests COMMAND test_plan_revision_cloud_editor)

add_executable(test_plan_revision_cloud_tool editor/tests/test_plan_revision_cloud_tool.c)
target_link_libraries(test_plan_revision_cloud_tool PRIVATE sitehelper_editor)
add_test(NAME plan_revision_cloud_tool_tests COMMAND test_plan_revision_cloud_tool)

add_executable(test_plan_dimension_tool editor/tests/test_plan_dimension_tool.c)
target_link_libraries(test_plan_dimension_tool PRIVATE sitehelper_editor)
add_test(NAME plan_dimension_tool_tests COMMAND test_plan_dimension_tool)


add_executable(test_plan_note_render app/tests/test_plan_note_render.c)
target_link_libraries(test_plan_note_render PRIVATE sitehelper_app_core)
add_test(NAME plan_note_render_tests COMMAND test_plan_note_render)

add_executable(test_plan_dimension_render app/tests/test_plan_dimension_render.c)
target_link_libraries(test_plan_dimension_render PRIVATE sitehelper_app_core)
add_test(NAME plan_dimension_render_tests COMMAND test_plan_dimension_render)

add_executable(test_plan_symbol_render app/tests/test_plan_symbol_render.c)
target_link_libraries(test_plan_symbol_render PRIVATE sitehelper_app_core)
add_test(NAME plan_symbol_render_tests COMMAND test_plan_symbol_render)

add_executable(test_plan_callout_render app/tests/test_plan_callout_render.c)
target_link_libraries(test_plan_callout_render PRIVATE sitehelper_app_core)
add_test(NAME plan_callout_render_tests COMMAND test_plan_callout_render)

add_executable(test_plan_revision_cloud_render app/tests/test_plan_revision_cloud_render.c)
target_link_libraries(test_plan_revision_cloud_render PRIVATE sitehelper_app_core)
add_test(NAME plan_revision_cloud_render_tests COMMAND test_plan_revision_cloud_render)

add_executable(test_text_input gui/tests/test_text_input.c)
target_link_libraries(test_text_input PRIVATE sitehelper_gui)
add_test(NAME text_input_tests COMMAND test_text_input)

add_executable(test_app_input app/tests/test_app_input.c)
target_link_libraries(test_app_input PRIVATE sitehelper_app_core)
add_test(NAME app_input_tests COMMAND test_app_input)

add_executable(test_slab_property_input app/tests/test_slab_property_input.c)
target_link_libraries(test_slab_property_input PRIVATE sitehelper_app_core)
add_test(NAME slab_property_input_tests COMMAND test_slab_property_input)

add_executable(test_sdl_input app/tests/test_sdl_input.c app/viewport_input.c)
target_link_libraries(test_sdl_input PRIVATE sitehelper_app_core sitehelper_persistence renderer2d_sdl platform_event_sdl SDL3::SDL3)
add_test(NAME sdl_input_tests COMMAND test_sdl_input)

add_executable(test_cli_measurements cli/tests/test_cli_measurements.c cli/actions.c)
target_include_directories(test_cli_measurements PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cli)
target_link_libraries(test_cli_measurements PRIVATE sitehelper_app_core sitehelper_gui)
add_test(NAME cli_measurement_tests COMMAND test_cli_measurements)

add_executable(test_measurement_tool editor/tests/test_measurement_tool.c)
target_link_libraries(test_measurement_tool PRIVATE sitehelper_editor)
add_test(NAME measurement_tool_tests COMMAND test_measurement_tool)

add_executable(test_editor_measurement editor/tests/test_editor_measurement.c)
target_link_libraries(test_editor_measurement PRIVATE sitehelper_app_core sitehelper_persistence)
add_test(NAME editor_measurement_tests COMMAND test_editor_measurement)

add_executable(test_plan_snap editor/tests/test_plan_snap.c)
target_link_libraries(test_plan_snap PRIVATE sitehelper_editor)
add_test(NAME plan_snap_tests COMMAND test_plan_snap)

add_executable(test_editor_plan_snap editor/tests/test_editor_plan_snap.c)
target_link_libraries(test_editor_plan_snap PRIVATE sitehelper_editor sitehelper_persistence)
add_test(NAME editor_plan_snap_tests COMMAND test_editor_plan_snap)
if(SITEHELPER_BUILD_TOPOLOGY)
    target_compile_definitions(test_plan_snap PRIVATE SITEHELPER_TEST_TOPOLOGY)
    target_link_libraries(test_plan_snap PRIVATE sitehelper_topology)
    target_compile_definitions(test_editor_plan_snap PRIVATE SITEHELPER_TEST_TOPOLOGY)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(test_plan_snap PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
        target_link_options(test_plan_snap PRIVATE
            "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
    endif()
endif()

add_executable(test_slab slab/tests/test_slab.c)
target_link_libraries(test_slab PRIVATE sitehelper_slab)
add_test(NAME slab_tests COMMAND test_slab)

add_executable(test_slab_property_edit slab/tests/test_slab_property_edit.c)
target_link_libraries(test_slab_property_edit PRIVATE sitehelper_slab)
add_test(NAME slab_property_edit_tests COMMAND test_slab_property_edit)

add_executable(test_slab_geometry_edit slab/tests/test_slab_geometry_edit.c)
target_link_libraries(test_slab_geometry_edit PRIVATE sitehelper_slab)
add_test(NAME slab_geometry_edit_tests COMMAND test_slab_geometry_edit)

add_executable(test_slab_plan_query slab_plan/tests/test_slab_plan_query.c)
target_link_libraries(test_slab_plan_query PRIVATE sitehelper_slab_plan_query)
add_test(NAME slab_plan_query_tests COMMAND test_slab_plan_query)

add_executable(test_slab_plan_render slab_plan/tests/test_slab_plan_render.c)
target_link_libraries(test_slab_plan_render PRIVATE sitehelper_app_core)
add_test(NAME slab_plan_render_tests COMMAND test_slab_plan_render)

add_executable(test_slab_plan_selection editor/tests/test_slab_plan_selection.c)
target_link_libraries(test_slab_plan_selection PRIVATE sitehelper_editor)
add_test(NAME slab_plan_selection_tests COMMAND test_slab_plan_selection)

add_executable(test_project_slabs project/tests/test_project_slabs.c)
target_link_libraries(test_project_slabs PRIVATE sitehelper_persistence sitehelper_test_support)
add_test(NAME project_slab_tests COMMAND test_project_slabs)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    foreach(slab_test test_slab test_project_slabs)
        target_compile_definitions(${slab_test} PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
        target_link_options(${slab_test} PRIVATE
            "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
    endforeach()
endif()

add_executable(test_slab_penetrations slab/tests/test_slab_penetrations.c)
target_link_libraries(test_slab_penetrations PRIVATE sitehelper_slab)
add_test(NAME slab_penetration_tests COMMAND test_slab_penetrations)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_penetrations PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_penetrations PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()

add_executable(test_project_slab_penetrations project/tests/test_project_slab_penetrations.c)
target_link_libraries(test_project_slab_penetrations PRIVATE sitehelper_persistence sitehelper_test_support)
add_test(NAME project_slab_penetration_tests COMMAND test_project_slab_penetrations)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_project_slab_penetrations PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_project_slab_penetrations PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()

add_executable(test_slab_regions slab/tests/test_slab_regions.c)
target_link_libraries(test_slab_regions PRIVATE sitehelper_slab)
add_test(NAME slab_region_tests COMMAND test_slab_regions)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_regions PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_regions PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()

add_executable(test_project_slab_regions project/tests/test_project_slab_regions.c)
target_link_libraries(test_project_slab_regions PRIVATE sitehelper_persistence sitehelper_test_support)
add_test(NAME project_slab_region_tests COMMAND test_project_slab_regions)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_project_slab_regions PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_project_slab_regions PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()

add_executable(test_slab_edge_rebates slab/tests/test_slab_edge_rebates.c)
target_link_libraries(test_slab_edge_rebates PRIVATE sitehelper_slab)
add_test(NAME slab_edge_rebate_tests COMMAND test_slab_edge_rebates)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_slab_edge_rebates PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_slab_edge_rebates PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=realloc")
endif()

add_executable(test_project_slab_edge_rebates project/tests/test_project_slab_edge_rebates.c)
target_link_libraries(test_project_slab_edge_rebates PRIVATE sitehelper_persistence sitehelper_test_support)
add_test(NAME project_slab_edge_rebate_tests COMMAND test_project_slab_edge_rebates)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(test_project_slab_edge_rebates PRIVATE SITEHELPER_TEST_WRAP_ALLOC)
    target_link_options(test_project_slab_edge_rebates PRIVATE
        "LINKER:--wrap=malloc" "LINKER:--wrap=calloc" "LINKER:--wrap=realloc")
endif()

add_executable(test_project_revision_clouds project/tests/test_project_revision_clouds.c)
target_link_libraries(test_project_revision_clouds PRIVATE sitehelper_project sitehelper_persistence)
add_test(NAME project_revision_cloud_tests COMMAND test_project_revision_clouds)

add_executable(test_project_revisions project/tests/test_project_revisions.c)
target_link_libraries(test_project_revisions PRIVATE sitehelper_project sitehelper_persistence)
add_test(NAME project_revision_tests COMMAND test_project_revisions)
