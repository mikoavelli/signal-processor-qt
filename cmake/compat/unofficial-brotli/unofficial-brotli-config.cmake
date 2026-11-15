# Compat package for Qt's FindWrapBrotli: the MinGW sysroot ships no
# unofficial-brotli CMake config, so point the expected targets at the
# static archives. Requires MINGW_SYSROOT to be set in the environment.
if(TARGET unofficial::brotli::brotlidec)
    set(unofficial-brotli_FOUND TRUE)
    return()
endif()

if(NOT DEFINED ENV{MINGW_SYSROOT})
    message(FATAL_ERROR "MINGW_SYSROOT is not set")
endif()
set(_brotli_prefix "$ENV{MINGW_SYSROOT}")

foreach(lib_name IN ITEMS brotlicommon brotlidec brotlienc)
    add_library(unofficial::brotli::${lib_name} STATIC IMPORTED)
    set_target_properties(unofficial::brotli::${lib_name} PROPERTIES
        IMPORTED_LOCATION "${_brotli_prefix}/lib/lib${lib_name}.a"
        INTERFACE_INCLUDE_DIRECTORIES "${_brotli_prefix}/include")
endforeach()

set_property(TARGET unofficial::brotli::brotlidec APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES unofficial::brotli::brotlicommon)
set_property(TARGET unofficial::brotli::brotlienc APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES unofficial::brotli::brotlicommon)

set(unofficial-brotli_FOUND TRUE)
