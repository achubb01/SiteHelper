add_library(sitehelper_core
    sitehelper.c
    appstate.c
)

target_include_directories(sitehelper_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
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