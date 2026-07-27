add_library(geometry
    geometry.c
)

target_include_directories(geometry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_geometry
    test_geometry.c
)

target_link_libraries(test_geometry
    PRIVATE
        geometry
)

if(NOT MSVC)
    target_link_libraries(test_geometry
        PRIVATE
            m
    )
endif()

add_test(
    NAME geometry_tests
    COMMAND test_geometry
)