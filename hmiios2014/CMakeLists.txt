
file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS *.cpp *.h *ui *.rc *.qrc)

hmiios_add_app_executable(NAME HMIIOS WIN32
    SRC ${PROJECT_SOURCES}
    QT5LIBS Core Gui Widgets Network OpenGLWidgets
)

# Copy sgMap files to the build output directory
add_custom_command(TARGET HMIIOS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:HMIIOS>/sgMap
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/sgMap
        $<TARGET_FILE_DIR:HMIIOS>/sgMap
)

# Add shapelib support if found
if(SHAPELIB_FOUND)
    target_include_directories(HMIIOS PRIVATE ${SHAPELIB_INCLUDE_DIR})
    target_link_libraries(HMIIOS PRIVATE ${SHAPELIB_LIBRARY})
else()
    # If shapelib is not found, we'll need to disable shapefile functionality
    message(WARNING "Shapelib not found. Shapefile support will be disabled.")
endif()

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
