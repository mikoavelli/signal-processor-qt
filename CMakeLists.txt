cmake_minimum_required(VERSION 3.21)

project(signal-processor-qt
    VERSION 1.0.0
    DESCRIPTION "SignalProcessor in Qt: spectrum analysis of communication lines"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release presets promise -O2 regardless of the toolchain's default release
# level (some CMake versions/distributions default to -O3).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    string(REGEX REPLACE "-O[0-9]+|-Os" "-O2" CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# Language of the build: en.json / ru.json ... from resources/i18n is compiled
# into the binary. A release in another language is produced by changing this
# option; the layout of the deliverables does not change: binary + settings.json.
set(SIGNAL_PROCESSOR_LANGUAGE "ru" CACHE STRING "Interface language compiled into the binary")
set(SIGNAL_PROCESSOR_I18N_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n/${SIGNAL_PROCESSOR_LANGUAGE}.json)
if(NOT EXISTS ${SIGNAL_PROCESSOR_I18N_FILE})
    message(FATAL_ERROR "Translation resources/i18n/${SIGNAL_PROCESSOR_LANGUAGE}.json not found")
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

# Translation compiled into the binary: resource path inside a single json.
target_compile_definitions(signal-processor-qt PRIVATE
    SIGNAL_PROCESSOR_TRANSLATION=":/i18n/${SIGNAL_PROCESSOR_LANGUAGE}.json"
)
set_property(SOURCE "${SIGNAL_PROCESSOR_I18N_FILE}" PROPERTY QT_RESOURCE_ALIAS
    "${SIGNAL_PROCESSOR_LANGUAGE}.json")
qt_add_resources(signal-processor-qt signalprocessor-i18n
    PREFIX "/i18n"
    FILES ${SIGNAL_PROCESSOR_I18N_FILE}
)

target_link_libraries(signal-processor-qt PRIVATE Qt6::Widgets Qt6::Sql)

# Static MinGW dependencies of the system freetype/harfbuzz that Qt's wrap does
# not know (freetype needs brotli even with a stock build).
if(WIN32)
    target_link_libraries(signal-processor-qt PRIVATE graphite2 bz2
        brotlidec brotlienc brotlicommon)
endif()

# Windows deliverable is named SignalProcessor.exe (see the release bundle).
if(WIN32)
    set_target_properties(signal-processor-qt PROPERTIES
        OUTPUT_NAME SignalProcessor
    )
    if(MINGW)
        # The static plugin packages are not findable via find_package (their
        # configs live inside the Qt6Gui/Qt6Sql package directories), so include
        # them directly: the Windows platform plugin and the SQLite driver that
        # measurements.db needs at runtime.
        set(SIGNAL_PROCESSOR_QT6_PLUGINS_DIR "/usr/x86_64-w64-mingw32/static/lib/cmake"
            CACHE PATH "Location of the static Qt plugin config packages (MinGW)")
        include(${SIGNAL_PROCESSOR_QT6_PLUGINS_DIR}/Qt6Gui/Qt6QWindowsIntegrationPluginConfig.cmake)
        include(${SIGNAL_PROCESSOR_QT6_PLUGINS_DIR}/Qt6Sql/Qt6QSQLiteDriverPluginConfig.cmake)
        # Link the plugin libraries and their initializer objects directly. The
        # Sql module's plugin auto-inclusion is disabled (it would require
        # PostgreSQL/MySQL), so the finalizer would never collect the SQLite
        # driver; explicit linkage is the robust substitute for the manual import.
        target_link_libraries(signal-processor-qt PRIVATE
            Qt6::QWindowsIntegrationPlugin Qt6::QWindowsIntegrationPlugin_init
            Qt6::QSQLiteDriverPlugin Qt6::QSQLiteDriverPlugin_init)
        unset(SIGNAL_PROCESSOR_QT6_PLUGINS_DIR)
    endif()
endif()

set_target_properties(signal-processor-qt PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

# -----------------------------------------------------------------------------
# Warnings and optimizations
# -----------------------------------------------------------------------------

include(CheckIPOSupported)

option(SIGNAL_PROCESSOR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(SIGNAL_PROCESSOR_LTO "Enable interprocedural (link-time) optimization" ON)
option(SIGNAL_PROCESSOR_NATIVE_OPT "Optimize for the build machine (-march=native), non-portable" OFF)
# Development helper for Linux: SIGNAL_PROCESSOR_SANITIZERS=ON adds AddressSanitizer
# and UndefinedBehaviorSanitizer (GCC/Clang only). Use it in a separate build
# directory, together with the Debug build type.
option(SIGNAL_PROCESSOR_SANITIZERS "Build with ASan + UBSan (GCC/Clang only)" OFF)

# Strict diagnostics. GCC/Clang and MSVC have different spellings; the flags
# are applied only to our sources, never to Qt itself.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SIGNAL_PROCESSOR_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        # Conversion and narrowing.
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wfloat-equal
        # Correctness style rules.
        -Wshadow
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wold-style-cast
        -Wcast-align
        -Wnull-dereference
        -Wvla
        -Warray-bounds=2
        -Wstringop-overflow=4
        # printf-style format and diagnostics.
        -Wformat=2
        -Wimplicit-fallthrough=5
        # Hardening.
        -fstack-protector-strong
        -fno-common
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC-only checks, Clang rejects some of them.
        list(APPEND SIGNAL_PROCESSOR_WARNING_FLAGS
            -Wlogical-op
            -Wduplicated-cond
            -Wduplicated-branches
            -Wuseless-cast
        )
    endif()
elseif(MSVC)
    # WinMain entry: keep WIN32_EXECUTABLE effective and avoid setlocale issues
    # with UTF-8 sources and resources.
    set(SIGNAL_PROCESSOR_WARNING_FLAGS
        /W4
        /permissive-
        /Zc:__cplusplus
        /Zc:preprocessor
        /utf-8
        # Signed/unsigned and narrowing equivalents.
        /w14242
        /w14254
        /w14263
        /w14265
        /w14287
        /w14296
        /w14311
        # Codegen parity for common suspects.
        /w14640
        /w14826
        /w14905
        /w14906
        /w14928
    )
    # Close to the GCC hardening set above.
    target_compile_options(signal-processor-qt PRIVATE /sdl) # extra lifetime/undef checks
endif()
if(SIGNAL_PROCESSOR_WARNINGS_AS_ERRORS)
    if(MSVC)
        list(APPEND SIGNAL_PROCESSOR_WARNING_FLAGS /WX)
    else()
        list(APPEND SIGNAL_PROCESSOR_WARNING_FLAGS -Werror)
    endif()
endif()
if(SIGNAL_PROCESSOR_WARNING_FLAGS)
    target_compile_options(signal-processor-qt PRIVATE ${SIGNAL_PROCESSOR_WARNING_FLAGS})
endif()

# Build machine tuning, opt-in: the produced binary stops being portable.
# MSVC equivalent (/arch:) is intentionally not enabled by default.
if(SIGNAL_PROCESSOR_NATIVE_OPT AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(signal-processor-qt PRIVATE -march=native)
endif()

# Sanitizers for the development builds on Linux (mutually exclusive with LTO
# and with the optimized release flags).
if(SIGNAL_PROCESSOR_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Sanitizers enabled: ASan + UBSan (development build)")
    target_compile_options(signal-processor-qt PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        $<$<CONFIG:Debug>:-fno-sanitize=signed-integer-overflow>)
    target_link_options(signal-processor-qt PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
endif()

# Release default of -O2 is fine; LTO buys a bit more.
# Qt's third-party templates sometimes warn, so warnings apply only to our
# sources (compile options on the target, not INTERFACE).
if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel")
    check_ipo_supported(RESULT SIGNAL_PROCESSOR_IPO OUTPUT SIGNAL_PROCESSOR_IPO_MSG)
    if(SIGNAL_PROCESSOR_LTO AND SIGNAL_PROCESSOR_IPO)
        set_target_properties(signal-processor-qt PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE)
    elseif(SIGNAL_PROCESSOR_LTO)
        message(STATUS "LTO not enabled (${SIGNAL_PROCESSOR_IPO_MSG})")
    endif()
endif()

# -----------------------------------------------------------------------------
# Tests (Qt Test + CTest)
# -----------------------------------------------------------------------------

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

    set(SIGNAL_PROCESSOR_UI_TESTABLE_SOURCES
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
        )
        # Reuse the same translation resource so tests exercise the real strings.
        set_property(SOURCE "${SIGNAL_PROCESSOR_I18N_FILE}" PROPERTY QT_RESOURCE_ALIAS
            "${SIGNAL_PROCESSOR_LANGUAGE}.json")
        qt_add_resources(${name} ${name}-i18n
            PREFIX "/i18n"
            FILES ${SIGNAL_PROCESSOR_I18N_FILE}
        )
        target_link_libraries(${name} PRIVATE Qt6::Test Qt6::Widgets Qt6::Sql)
        target_compile_options(${name} PRIVATE ${SIGNAL_PROCESSOR_WARNING_FLAGS})
        # The tests resolve the shipped settings.json relative to the source tree.
        target_compile_definitions(${name} PRIVATE
            SIGNAL_PROCESSOR_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME ${name} COMMAND ${name})
        set_tests_properties(${name} PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endfunction()

    signal_processor_add_test(test_spectrum_reader ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_calculation ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_settings_store ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_database ${SIGNAL_PROCESSOR_TESTABLE_SOURCES})
    signal_processor_add_test(test_ui ${SIGNAL_PROCESSOR_UI_TESTABLE_SOURCES})
endif()
