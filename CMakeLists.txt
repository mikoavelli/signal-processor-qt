cmake_minimum_required(VERSION 3.21)

project(signal-processor-qt
    VERSION 1.0.0
    DESCRIPTION "SignalProcessor in Qt: spectrum analysis of communication lines"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release presets promise -O2 regardless of the toolchain's default release
# level (some CMake versions default to -O3).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    string(REGEX REPLACE "-O[0-9]+|-Os" "-O2" CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# Interface language compiled into the binary; a build in another language is
# produced by changing SIGNAL_PROCESSOR_LANGUAGE.
set(SIGNAL_PROCESSOR_LANGUAGE "ru" CACHE STRING "Interface language")
set(SIGNAL_PROCESSOR_I18N_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n/${SIGNAL_PROCESSOR_LANGUAGE}.json)
if(NOT EXISTS ${SIGNAL_PROCESSOR_I18N_FILE})
    message(FATAL_ERROR "Translation ${SIGNAL_PROCESSOR_I18N_FILE} not found")
endif()

find_package(Qt6 6.12 REQUIRED COMPONENTS Widgets Sql)
qt_standard_project_setup(REQUIRES 6.12)

set(SIGNAL_PROCESSOR_SOURCES
    src/main.cpp
    src/i18n.h
    src/i18n.cpp
    src/errors.h
    src/errors.cpp
    src/models.h
    src/models.cpp
    src/settings_store.h
    src/settings_store.cpp
    src/spectrum_reader.h
    src/spectrum_reader.cpp
    src/calculation.h
    src/calculation.cpp
    src/database.h
    src/database.cpp
    src/ui/double_validator.h
    src/ui/double_validator.cpp
    src/ui/file_drop_line_edit.h
    src/ui/file_drop_line_edit.cpp
    src/ui/line_template_widget.h
    src/ui/line_template_widget.cpp
    src/ui/table_model.h
    src/ui/table_model.cpp
    src/ui/startup_dialog.h
    src/ui/startup_dialog.cpp
    src/ui/main_window.h
    src/ui/main_window.cpp
)
qt_add_executable(signal-processor-qt
    ${SIGNAL_PROCESSOR_SOURCES}
)
target_include_directories(signal-processor-qt PRIVATE src src/ui)

# Translation compiled into the binary.
target_compile_definitions(signal-processor-qt PRIVATE
    SIGNAL_PROCESSOR_TRANSLATION=":/i18n/${SIGNAL_PROCESSOR_LANGUAGE}.json")
set_property(SOURCE "${SIGNAL_PROCESSOR_I18N_FILE}" PROPERTY QT_RESOURCE_ALIAS
    "${SIGNAL_PROCESSOR_LANGUAGE}.json")
qt_add_resources(signal-processor-qt signalprocessor-i18n
    PREFIX "/i18n"
    FILES ${SIGNAL_PROCESSOR_I18N_FILE}
)

target_link_libraries(signal-processor-qt PRIVATE Qt6::Widgets Qt6::Sql)

# MinGW links static system libraries for the Qt wraps that Qt does not know.
if(WIN32)
    target_link_libraries(signal-processor-qt PRIVATE graphite2 bz2
        brotlidec brotlienc brotlicommon)
endif()

# Windows deliverable is named SignalProcessor.exe; static Qt needs its platform
# and SQL plugins linked explicitly.
if(WIN32)
    set_target_properties(signal-processor-qt PROPERTIES
        OUTPUT_NAME SignalProcessor
        WIN32_EXECUTABLE TRUE)
    if(MINGW
        AND EXISTS "${STATIC_QT_PREFIX}/lib/cmake/Qt6Gui/Qt6QWindowsIntegrationPluginConfig.cmake")
        set(SIGNAL_PROCESSOR_QT6_PLUGINS_DIR "${STATIC_QT_PREFIX}/lib/cmake"
            CACHE PATH "Static Qt plugin config packages (MinGW)")
        include(${SIGNAL_PROCESSOR_QT6_PLUGINS_DIR}/Qt6Gui/Qt6QWindowsIntegrationPluginConfig.cmake)
        include(${SIGNAL_PROCESSOR_QT6_PLUGINS_DIR}/Qt6Sql/Qt6QSQLiteDriverPluginConfig.cmake)
        target_link_libraries(signal-processor-qt PRIVATE
            Qt6::QWindowsIntegrationPlugin Qt6::QWindowsIntegrationPlugin_init
            Qt6::QSQLiteDriverPlugin Qt6::QSQLiteDriverPlugin_init)
        unset(SIGNAL_PROCESSOR_QT6_PLUGINS_DIR)
    endif()
else()
    set_target_properties(signal-processor-qt PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE FALSE
    )
endif()

# Strict diagnostics: applied only to our sources, never to Qt itself.
set(SIGNAL_PROCESSOR_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wfloat-equal
    -Wshadow
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wold-style-cast
    -Wcast-align
    -Wnull-dereference
    -Wvla
    -Warray-bounds=2
    -Wstringop-overflow=4
    -Wformat=2
    -Wimplicit-fallthrough=5
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND SIGNAL_PROCESSOR_WARNING_FLAGS
        -Wlogical-op
        -Wduplicated-cond
        -Wduplicated-branches
        -Wuseless-cast
    )
endif()
option(SIGNAL_PROCESSOR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
if(SIGNAL_PROCESSOR_WARNINGS_AS_ERRORS)
    list(APPEND SIGNAL_PROCESSOR_WARNING_FLAGS -Werror)
endif()
target_compile_options(signal-processor-qt PRIVATE ${SIGNAL_PROCESSOR_WARNING_FLAGS})

# LTO in release builds; best effort, gains a bit of speed.
if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT SIGNAL_PROCESSOR_IPO)
    if(SIGNAL_PROCESSOR_IPO)
        set_target_properties(signal-processor-qt PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# Qt Test + CTest suite. Shares the application sources.
option(SIGNAL_PROCESSOR_BUILD_TESTS "Build the Qt Test suite" ON)
if(SIGNAL_PROCESSOR_BUILD_TESTS AND NOT WIN32)
    enable_testing()
    find_package(Qt6 6.12 REQUIRED COMPONENTS Test)

    set(SIGNAL_PROCESSOR_TESTABLE_SOURCES
        src/i18n.cpp
        src/errors.cpp
        src/models.cpp
        src/settings_store.cpp
        src/spectrum_reader.cpp
        src/calculation.cpp
        src/database.cpp
    )
    set(SIGNAL_PROCESSOR_UI_SOURCES
        ${SIGNAL_PROCESSOR_TESTABLE_SOURCES}
        src/ui/double_validator.cpp
        src/ui/file_drop_line_edit.cpp
        src/ui/line_template_widget.cpp
        src/ui/table_model.cpp
    )

    function(signal_processor_add_test name)
        qt_add_executable(${name} tests/${name}.cpp ${ARGN})
        target_include_directories(${name} PRIVATE src src/ui)
        target_compile_definitions(${name} PRIVATE
            SIGNAL_PROCESSOR_TRANSLATION=":/i18n/${SIGNAL_PROCESSOR_LANGUAGE}.json"
            SIGNAL_PROCESSOR_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        set_property(SOURCE "${SIGNAL_PROCESSOR_I18N_FILE}" PROPERTY QT_RESOURCE_ALIAS
            "${SIGNAL_PROCESSOR_LANGUAGE}.json")
        qt_add_resources(${name} ${name}-i18n
            PREFIX "/i18n"
            FILES ${SIGNAL_PROCESSOR_I18N_FILE}
        )
        target_link_libraries(${name} PRIVATE Qt6::Test Qt6::Widgets Qt6::Sql)
        target_compile_options(${name} PRIVATE ${SIGNAL_PROCESSOR_WARNING_FLAGS})
        add_test(NAME ${name} COMMAND ${name})
        set_tests_properties(${name} PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endfunction()

    signal_processor_add_test(test_spectrum_reader ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_calculation ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_settings_store ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_database ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_ui ${SIGNAL_PROCESSOR_UI_SOURCES})
endif()
