add_executable(appConfigTests
    appConfigTests.cpp
    "${APP_CONFIG_GENERATED_SOURCE}"
    "${APP_CONFIG_GENERATED_HEADER}"
    "${APP_CONFIG_EVENTS}"
)

add_dependencies(appConfigTests generateAppConfig)
target_include_directories(appConfigTests PRIVATE "${APP_CONFIG_GENERATED_DIR}")
target_link_libraries(appConfigTests PRIVATE Qt6::Core Qt6::Gui)
target_compile_definitions(appConfigTests PRIVATE APP_CONFIG_SCHEMA_PATH="${APP_CONFIG_SCHEMA}")

add_test(NAME appConfigTests COMMAND appConfigTests)