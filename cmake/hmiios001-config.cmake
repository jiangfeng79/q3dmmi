
if (CMAKE_VERSION VERSION_LESS 4.2.0)
    message(FATAL_ERROR "Hmiios framework requires at least CMake version 4.2.0")
endif()

# The following three lines will be set by dist-helper.py
# There is no need to change these by hand
set(HMIIOS_MAJOR_VERSION 0)
set(HMIIOS_MINOR_VERSION 0)
set(HMIIOS_PATCH_VERSION 1)

set(HMIIOS_QT_MAJOR_VERSION 6)

set(_HMIIOS_FULL "hmiios${HMIIOS_MAJOR_VERSION}${HMIIOS_MINOR_VERSION}${HMIIOS_PATCH_VERSION}")

function(hmiios_add_app_library)
    cmake_parse_arguments(
    parsered
    "STATIC"
    "NAME;OUTPUTDIR"
    "SRC;DEFINES;QTLIBS;QT5LIBS;HMIIOSLIBS;OTHERLIBS"
    ${ARGN})

    if (NOT parsered_NAME)
        message(FATAL_ERROR "You must provided a name")
    endif()

    set(LIBTYPE SHARED)
    if (parsered_STATIC)
        set(LIBTYPE STATIC)
    endif()

    add_library(${parsered_NAME} ${LIBTYPE} ${parsered_SRC})

    if (parsered_DEFINES)
        add_compile_definitions(${parsered_DEFINES})
    endif()
    if (parsered_QT5LIBS)
        # in order to make old code work
        # remove me when all code port to QTLIBS
        set(parsered_QTLIBS ${parsered_QTLIBS} ${parsered_QT5LIBS})
    endif()
    if (parsered_QTLIBS)
	    find_package(Qt${HMIIOS_QT_MAJOR_VERSION} COMPONENTS ${parsered_QTLIBS} REQUIRED)
        foreach(_lib ${parsered_QTLIBS})
            target_link_libraries(${parsered_NAME} PRIVATE Qt::${_lib})
        endforeach()
    endif()
    if (parsered_HMIIOSLIBS)
        find_package(${_HMIIOS_FULL} COMPONENTS ${parsered_HMIIOSLIBS} REQUIRED)
        foreach(_lib ${parsered_HMIIOSLIBS})
            target_link_libraries(${parsered_NAME} PRIVATE hmiios::${_lib})
        endforeach()
    endif()
    if (parsered_OTHERLIBS)
        foreach(_lib ${parsered_OTHERLIBS})
            target_link_libraries(${parsered_NAME} PRIVATE ${_lib})
        endforeach()
    endif()
    set_target_properties(${parsered_NAME} PROPERTIES
        DEBUG_POSTFIX "${HMIIOS_LIB_DEBUG_POSTFIX}"
    )
    if (parsered_OUTPUTDIR)
        set_target_properties(${parsered_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${parsered_OUTPUTDIR}"
        )
    endif()

    set_property(TARGET ${parsered_NAME} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/..)
endfunction()

function(hmiios_add_app_executable)
    cmake_parse_arguments(
    parsered
    "WIN32"
    "NAME;OUTPUTDIR"
    "SRC;DEFINES;QTLIBS;QT5LIBS;HMIIOSLIBS;OTHERLIBS"
    ${ARGN})

    if (NOT parsered_NAME)
        message(FATAL_ERROR "You must provided a name")
    endif()

    if (parsered_WIN32)
        set(LINKTYPE WIN32)
    endif()

    add_executable(${parsered_NAME} ${LINKTYPE} ${parsered_SRC})

    if (parsered_DEFINES)
        add_compile_definitions(${parsered_DEFINES})
    endif()
    if (parsered_QT5LIBS)
        # in order to make old code work
        # remove me when all code port to QTLIBS
        set(parsered_QTLIBS ${parsered_QTLIBS} ${parsered_QT5LIBS})
    endif()
    if (parsered_QTLIBS)
	    find_package(Qt${HMIIOS_QT_MAJOR_VERSION} COMPONENTS ${parsered_QTLIBS} REQUIRED)
        foreach(_lib ${parsered_QTLIBS})
            target_link_libraries(${parsered_NAME} PRIVATE Qt::${_lib})
        endforeach()
    endif()
    if (parsered_HMIIOSLIBS)
        find_package(${_HMIIOS_FULL} COMPONENTS ${parsered_HMIIOSLIBS} REQUIRED)
        foreach(_lib ${parsered_HMIIOSLIBS})
            target_link_libraries(${parsered_NAME} PRIVATE hmiios::${_lib})
        endforeach()
    endif()
    if (parsered_OTHERLIBS)
        foreach(_lib ${parsered_OTHERLIBS})
            target_link_libraries(${parsered_NAME} PRIVATE ${_lib})
        endforeach()
    endif()
    if (parsered_OUTPUTDIR)
        set_target_properties(${parsered_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${parsered_OUTPUTDIR}"
        )
    endif()
endfunction()

#
# Be careful, the function will be used cross different repositories
# 
function(hmiios_add_device_library)
    cmake_parse_arguments(
    parsered
    ""
    "BASENAME;OUTPUTDIR"
    "SRC;DEFINES;QTLIBS;QT5LIBS;HMIIOSLIBS;OTHERLIBS"
    ${ARGN})

    if (NOT parsered_BASENAME)
        message(FATAL_ERROR "You must provided a basename")
    endif()

    set(name hmiiosdevice${parsered_BASENAME})
    string(TOUPPER ${parsered_BASENAME} uppername)
    add_compile_definitions()


    hmiios_add_app_library(NAME ${name}
        SRC ${parsered_SRC}
        DEFINES HMIIOSDEVICE_BUILD_${uppername}_LIB ${parsered_DEFINES}
        QTLIBS ${parsered_QTLIBS} ${parsered_QT5LIBS}
        HMIIOSLIBS ${parsered_HMIIOSLIBS}
        OTHERLIBS ${parsered_OTHERLIBS}
        OUTPUTDIR ${parsered_OUTPUTDIR}
    )
endfunction()

# Set some common variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
if (MSVC)
    set(CMAKE_C_FLAGS "/utf-8 ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "/utf-8 ${CMAKE_CXX_FLAGS}")
endif()
