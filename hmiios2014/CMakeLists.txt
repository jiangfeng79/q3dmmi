
file(GLOB PROJECT_SOURCES *.cpp *.h *ui *.rc *.qrc)

hmiios_add_app_executable(NAME HMIIOS WIN32
    SRC ${PROJECT_SOURCES}
    QT5LIBS Core Gui Widgets Network OpenGLWidgets
)

target_include_directories(HMIIOS PRIVATE ${PROJECT_SOURCE_DIR}/../shapelib-1.5.0 )
target_link_directories(HMIIOS PRIVATE ${PROJECT_SOURCE_DIR}/../shapelib-1.5.0)
target_link_libraries(HMIIOS PRIVATE shapelib)

if(WIN32)
    target_link_libraries(HMIIOS PRIVATE opengl32)
endif()

if(MSVC)
    set_property(TARGET HMIIOS
        PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}/hmiios2014
        )
else()
    set(DEBUG_WORKING_DIR "${CMAKE_BINARY_DIR}/bin")
endif()


set_target_properties(HMIIOS PROPERTIES FOLDER "Main")
