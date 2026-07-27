add_library(commandmenu
    commandmenu.c
)

target_include_directories(commandmenu
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_commandmenu
    test_commandmenu.c
)

target_link_libraries(test_commandmenu
    PRIVATE
        commandmenu
)

add_test(
    NAME commandmenu_tests
    COMMAND test_commandmenu
)