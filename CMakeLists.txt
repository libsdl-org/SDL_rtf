cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# See docs/release_checklist.md
set(MAJOR_VERSION 3)
set(MINOR_VERSION 0)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION 3.0.0)

set(SDLTTF_REQUIRED_VERSION 3.0.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the SDL_rtf source code and call cmake from there")
endif()

project(SDL3_rtf
    LANGUAGES C
    VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
)

include("${SDL3_rtf_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake" )
include("${SDL3_rtf_SOURCE_DIR}/cmake/PrivateSdlFunctions.cmake")
include("${SDL3_rtf_SOURCE_DIR}/cmake/sdlmanpages.cmake")
sdl_calculate_derived_version_variables(${MAJOR_VERSION} ${MINOR_VERSION} ${MICRO_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SDL3RTF_ROOTPROJECT ON)
else()
    set(SDL3RTF_ROOTPROJECT OFF)
endif()

# Assume MSVC projects don't have a package manager and need vendored dependencies (by default).
# Most other platforms have some kind of package manager.
# FIXME: consider a package manager such as conan/vcpkg instead of vendoring
if(ANDROID OR MSVC)
    set(vendored_default TRUE)
else()
    set(vendored_default FALSE)
endif()

set(sdl3rtf_install_enableable ON)
if ((TARGET SDL3-shared OR TARGET SDL3-static OR TARGET SDL3_mixer-shared OR TARGET SDL3_mixer-static) AND SDL_DISABLE_INSTALL)
    # Cannot install SDL3_rtf when SDL3 is built in same built, and is not installed.
    set(sdl3rtf_install_enableable OFF)
endif()

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CheckSymbolExists)

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ON)
option(BUILD_SHARED_LIBS "Build the library as a shared library" ON)

cmake_dependent_option(SDL3RTF_INSTALL "Enable SDL3_rtf install target" ${SDL3RTF_ROOTPROJECT} "${sdl3rtf_install_enableable}" OFF)
cmake_dependent_option(SDL3RTF_INSTALL_MAN "Install man pages for SDL3_rtf" ${SDL3RTF_ROOTPROJECT} "SDL3RTF_INSTALL" OFF)
option(SDL3RTF_WERROR "Treat warnings as errors" OFF)

option(SDL3RTF_SAMPLES "Build the SDL3_rtf sample program(s)" ${SDL3RTF_ROOTPROJECT})
cmake_dependent_option(SDL3RTF_SAMPLES_INSTALL "Install the SDL3_rtf sample program(s)" OFF "SDL3RTF_SAMPLES;SDL3RTF_INSTALL" OFF)

# Save BUILD_SHARED_LIBS variable
set(SDL3RTF_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")

set(sdl_required_components Headers)

if(SDL3RTF_BUILD_SHARED_LIBS)
    set(sdl3_rtf_target_name SDL3_rtf-shared)
    set(sdl3_target_name SDL3::SDL3-shared)

    list(APPEND sdl_required_components SDL3-shared)

    set(sdl3ttf_target_name SDL3_ttf::SDL3_ttf-shared)
else()
    set(sdl3_rtf_target_name SDL3_rtf-static)
    set(sdl3_target_name SDL3::SDL3)

    set(sdl3ttf_target_name SDL3_ttf::SDL3_ttf)
endif()

if(NOT TARGET SDL3::Headers OR NOT TARGET ${sdl3_target_name})
    find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED COMPONENTS ${sdl_required_components})
endif()

if(NOT TARGET ${sdl3ttf_target_name})
    find_package(SDL3_ttf REQUIRED)
endif()

add_library(${sdl3_rtf_target_name}
    src/rtfactn.c
    src/rtfreadr.c
    src/SDL_rtf.c
    src/SDL_rtfreadr.c
)
add_library(SDL3_rtf::${sdl3_rtf_target_name} ALIAS ${sdl3_rtf_target_name})
if(NOT TARGET SDL3_rtf::SDL3_rtf)
    add_library(SDL3_rtf::SDL3_rtf ALIAS ${sdl3_rtf_target_name})
endif()
target_include_directories(${sdl3_rtf_target_name} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3_rtf>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/SDL3_rtf>"
)
target_compile_definitions(${sdl3_rtf_target_name} PRIVATE
    BUILD_SDL
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
)
target_link_libraries(${sdl3_rtf_target_name} PUBLIC SDL3::Headers)
if(SDL3RTF_BUILD_SHARED_LIBS)
    target_link_libraries(${sdl3_rtf_target_name} PRIVATE SDL3::SDL3-shared)
endif()
sdl_add_warning_options(${sdl3_rtf_target_name} WARNING_AS_ERROR ${SDL3RTF_WERROR})
if(WIN32 AND SDL3RTF_BUILD_SHARED_LIBS)
    target_sources(${sdl3_rtf_target_name} PRIVATE
        src/version.rc
    )
endif()
set_target_properties(${sdl3_rtf_target_name} PROPERTIES
    OUTPUT_NAME SDL3_rtf
    DEFINE_SYMBOL DLL_EXPORT
    EXPORT_NAME ${sdl3_rtf_target_name}
    C_VISIBILITY_PRESET "hidden"
)
if(NOT ANDROID)
    if(APPLE)
        # the SOVERSION property corresponds to the compatibility version and VERSION corresponds to the current version
        # https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html#mach-o-versions
        set_target_properties(${sdl3_rtf_target_name} PROPERTIES
            SOVERSION "${DYLIB_COMPAT_VERSION}"
            VERSION "${DYLIB_CURRENT_VERSION}"
        )
    else()
        set_target_properties(${sdl3_rtf_target_name} PROPERTIES
            SOVERSION "${SO_VERSION_MAJOR}"
            VERSION "${SO_VERSION}"
        )
    endif()
endif()
if(SDL3RTF_BUILD_SHARED_LIBS)
    if(WIN32)
        set_target_properties(${sdl3_rtf_target_name} PROPERTIES
            PREFIX ""
        )
    endif()
else()
    if(MSVC)
        set_target_properties(${sdl3_rtf_target_name} PROPERTIES
            OUTPUT_NAME "SDL3_rtf-static"
            )
    endif()
endif()

if(SDL3RTF_BUILD_SHARED_LIBS)
    # Use `Compatible Interface Properties` to ensure a shared SDL3_rtf is linked to a shared SDL3 library
    set_property(TARGET ${sdl3_rtf_target_name} PROPERTY INTERFACE_SDL3_SHARED ${SDL3RTF_BUILD_SHARED_LIBS})
    set_property(TARGET ${sdl3_rtf_target_name} APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL3_SHARED)
endif()

if(SDL3RTF_BUILD_SHARED_LIBS)
    sdl_target_link_options_no_undefined(${sdl3_rtf_target_name})
endif()

sdl_target_link_option_version_file(${sdl3_rtf_target_name} "${CMAKE_CURRENT_SOURCE_DIR}/src/SDL_rtf.sym")

if(SDL3RTF_INSTALL)
    install(
        TARGETS ${sdl3_rtf_target_name}
        EXPORT SDL3_rtfTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3_rtf/SDL_rtf.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3_rtf" COMPONENT devel
    )

    if(WIN32 AND NOT MINGW)
        set(SDLRTF_INSTALL_CMAKEDIR_DEFAULT "cmake")
    else()
        set(SDLRTF_INSTALL_CMAKEDIR_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake/SDL3_rtf")
    endif()
    set(SDLRTF_INSTALL_CMAKEDIR "${SDLRTF_INSTALL_CMAKEDIR_DEFAULT}" CACHE STRING "Location where to install SDL3_rtfConfig.cmake")
    set(SDLRTF_PKGCONFIG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

    configure_package_config_file(cmake/SDL3_rtfConfig.cmake.in SDL3_rtfConfig.cmake
        INSTALL_DESTINATION "${SDLRTF_INSTALL_CMAKEDIR}"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL3_rtfConfigVersion.cmake"
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_rtfConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_rtfConfigVersion.cmake"
        DESTINATION ${SDLRTF_INSTALL_CMAKEDIR}
        COMPONENT devel
    )
    install(EXPORT SDL3_rtfTargets
        FILE "${sdl3_rtf_target_name}-targets.cmake"
        NAMESPACE SDL3_rtf::
        DESTINATION "${SDLRTF_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )

    file(RELATIVE_PATH SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${CMAKE_INSTALL_PREFIX}/${SDLRTF_PKGCONFIG_INSTALLDIR}" "${CMAKE_INSTALL_PREFIX}")
    string(REGEX REPLACE "[/]+$" "" SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
    set(SDL_PKGCONFIG_PREFIX "\${pcfiledir}/${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")

    configure_file(cmake/sdl3-rtf.pc.in sdl3-rtf.pc @ONLY)

    # Always install sdl3-rtf.pc file: libraries might be different between config modes
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl3-rtf.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig" COMPONENT devel)

    install(FILES "LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
        COMPONENT library
    )

    if(SDL3RTF_INSTALL_MAN)
        sdl_get_git_revision_hash(SDL3RTF_REVISION)
        SDL_generate_manpages(
            HEADERS_DIR "${PROJECT_DIR}/include/SDL3_rtf"
            SYMBOL "RTF_CreateContext"
            WIKIHEADERS_PL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-scripts/wikiheaders.pl"
            REVISION "${SDL3RTF_REVISION}"
        )
    endif()
endif()

if(SDL3RTF_SAMPLES)
    add_executable(showrtf examples/showrtf.c)

    foreach(prog showrtf)
        sdl_add_warning_options(${prog} WARNING_AS_ERROR ${SDL3RTF_WERROR})
        target_link_libraries(${prog} PRIVATE SDL3_rtf::${sdl3_rtf_target_name})
        target_link_libraries(${prog} PRIVATE ${sdl3ttf_target_name})
        target_link_libraries(${prog} PRIVATE ${sdl3_target_name})

        if(SDL3RTF_SAMPLES_INSTALL)
            install(TARGETS ${prog}
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            )
        endif()
    endforeach()
endif()
