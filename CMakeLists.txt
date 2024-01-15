cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# See docs/release_checklist.md
set(MAJOR_VERSION 2)
set(MINOR_VERSION 0)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION 2.0.0)

include(PrivateSdlFunctions)
sdl_calculate_derived_version_variables()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the SDL_rtf source code and call cmake from there")
endif()

project(SDL2_rtf
    LANGUAGES C
    VERSION "${FULL_VERSION}"
)

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SDL2RTF_ROOTPROJECT ON)
else()
    set(SDL2RTF_ROOTPROJECT OFF)
endif()

# Set defaults preventing destination file conflicts
set(SDL2RTF_DEBUG_POSTFIX "d"
    CACHE STRING "Name suffix for debug builds")
mark_as_advanced(SDL2RTF_DEBUG_POSTFIX)

# Assume MSVC projects don't have a package manager and need vendored dependencies (by default).
# Most other platforms have some kind of package manager.
# FIXME: consider a package manager such as conan/vcpkg instead of vendoring
if(MSVC)
    set(vendored_default TRUE)
else()
    set(vendored_default FALSE)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ON)
option(BUILD_SHARED_LIBS "Build the library as a shared library" ON)

# Save BUILD_SHARED_LIBS variable
set(SDL2RTF_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")

option(SDL2RTF_SAMPLES "Build the SDL2_rtf sample program(s)" ${SDL2RTF_ROOTPROJECT})
option(SDL2RTF_INSTALL "Enable SDL2_rtf install target" ${SDL2RTF_ROOTPROJECT})

if(SDL2RTF_BUILD_SHARED_LIBS)
    set(sdl2_rtf_export_name SDL2_rtf)
    set(sdl2_rtf_install_name_infix shared)
    set(sdl2_target_name SDL2::SDL2)
    set(sdl2ttf_target_name SDL2_ttf::SDL2_ttf)
else()
    set(sdl2_rtf_export_name SDL2_rtf-static)
    set(sdl2_rtf_install_name_infix static)
    set(sdl2_target_name SDL2::SDL2-static)
    set(sdl2ttf_target_name SDL2_ttf::SDL2_ttf-static)
endif()

sdl_find_sdl2(${sdl2_target_name} ${SDL_REQUIRED_VERSION})
if(NOT TARGET ${sdl2ttf_target_name})
    find_package(SDL2_ttf REQUIRED)
endif()

add_library(SDL2_rtf
    rtfactn.c
    rtfdecl.h
    rtfreadr.c
    rtftype.h
    SDL_rtf.c
    SDL_rtf.h
    SDL_rtfreadr.c
    SDL_rtfreadr.h
)
add_library(SDL2_rtf::${sdl2_rtf_export_name} ALIAS SDL2_rtf)
target_include_directories(SDL2_rtf PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>"
)
target_compile_definitions(SDL2_rtf PRIVATE
    BUILD_SDL
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
)
target_link_libraries(SDL2_rtf PRIVATE $<BUILD_INTERFACE:${sdl2_target_name}> $<BUILD_INTERFACE:${sdl2ttf_target_name}>)
if(WIN32 AND SDL2RTF_BUILD_SHARED_LIBS)
    target_sources(SDL2_rtf PRIVATE
        version.rc
    )
endif()
set_target_properties(SDL2_rtf PROPERTIES
    DEFINE_SYMBOL DLL_EXPORT
    EXPORT_NAME ${sdl2_rtf_export_name}
    C_VISIBILITY_PRESET "hidden"
)
if(NOT ANDROID)
    set_target_properties(SDL2_rtf PROPERTIES
        DEBUG_POSTFIX "${SDL2RTF_DEBUG_POSTFIX}"
    )
    if(APPLE)
        # the SOVERSION property corresponds to the compatibility version and VERSION corresponds to the current version
        # https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html#mach-o-versions
        set_target_properties(SDL2_rtf PROPERTIES
            SOVERSION "${DYLIB_COMPATIBILITY_VERSION}"
            VERSION "${DYLIB_CURRENT_VERSION}"
        )
    else()
        set_target_properties(SDL2_rtf PROPERTIES
            SOVERSION "${LT_MAJOR}"
            VERSION "${LT_VERSION}"
        )
    endif()
endif()
if(SDL2RTF_BUILD_SHARED_LIBS AND (APPLE OR (UNIX AND NOT ANDROID)))
    add_custom_command(TARGET SDL2_rtf POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E create_symlink "$<TARGET_SONAME_FILE_NAME:SDL2_rtf>" "libSDL2_rtf$<$<CONFIG:Debug>:${SDL2RTF_DEBUG_POSTFIX}>$<TARGET_FILE_SUFFIX:SDL2_rtf>"
        # BYPRODUCTS "libSDL2_rtf$<$<CONFIG:Debug>:${SDL2RTF_DEBUG_POSTFIX}>$<TARGET_FILE_SUFFIX:SDL2_rtf>" # Needs CMake 3.20
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    )
endif()
if(SDL2RTF_BUILD_SHARED_LIBS)
    if(WIN32 OR OS2)
        set_target_properties(SDL2_rtf PROPERTIES
            PREFIX ""
        )
    endif()
    if(OS2)
        # OS/2 doesn't support a DLL name longer than 8 characters.
        set_target_properties(SDL2_rtf PROPERTIES
            OUTPUT_NAME "SDL2rtf"
        )
    elseif(UNIX AND NOT ANDROID)
        set_target_properties(SDL2_rtf PROPERTIES
            OUTPUT_NAME "SDL2_rtf-${LT_RELEASE}"
        )
    endif()
endif()

if(SDL2RTF_BUILD_SHARED_LIBS)
    # Use `Compatible Interface Properties` to ensure a shared SDL2_Rtf is linked to a shared SDL2 library
    set_property(TARGET SDL2_rtf PROPERTY INTERFACE_SDL2_SHARED ${SDL2RTF_BUILD_SHARED_LIBS})
    set_property(TARGET SDL2_rtf APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL2_SHARED)
endif()

set(INSTALL_EXTRA_TARGETS)
set(PC_LIBS)
set(PC_REQUIRES SDL2_ttf)

if(SDL2RTF_INSTALL)
    install(
        TARGETS SDL2_rtf
        EXPORT SDL2_rtfTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/SDL_rtf.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL2" COMPONENT DEVEL
    )

    if(WIN32 AND NOT MINGW)
        set(SDLRTF_INSTALL_CMAKEDIR_DEFAULT "cmake")
    else()
        set(SDLRTF_INSTALL_CMAKEDIR_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_rtf")
    endif()
    set(SDLRTF_INSTALL_CMAKEDIR "${SDLRTF_INSTALL_CMAKEDIR_DEFAULT}" CACHE STRING "Location where to install SDL2_rtfConfig.cmake")

    configure_package_config_file(SDL2_rtfConfig.cmake.in SDL2_rtfConfig.cmake
        INSTALL_DESTINATION "${SDLRTF_INSTALL_CMAKEDIR}"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL2_rtfConfigVersion.cmake"
        VERSION ${FULL_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtfConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtfConfigVersion.cmake"
        DESTINATION ${SDLRTF_INSTALL_CMAKEDIR}
        COMPONENT devel
    )
    install(EXPORT SDL2_rtfTargets
        FILE SDL2_rtf-${sdl2_rtf_install_name_infix}-targets.cmake
        NAMESPACE SDL2_rtf::
        DESTINATION "${SDLRTF_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )

    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(exec_prefix "\${prefix}")
    set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    set(PACKAGE "${PROJECT_NAME}")
    set(VERSION ${FULL_VERSION})
    set(SDL_VERSION ${SDL_REQUIRED_VERSION})
    string(JOIN " " PC_REQUIRES ${PC_REQUIRES})
    string(JOIN " " PC_LIBS ${PC_LIBS})
    configure_file("${PROJECT_SOURCE_DIR}/SDL2_rtf.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtf.pc.intermediate" @ONLY)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtf-$<CONFIG>.pc" INPUT "${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtf.pc.intermediate")

    set(PC_DESTDIR)
    if(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
        # FreeBSD uses ${PREFIX}/libdata/pkgconfig
        set(PC_DESTDIR "libdata/pkgconfig")
    else()
        set(PC_DESTDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    endif()
    # Always install SDL2_rtf.pc file: libraries might be different between config modes
    install(CODE "
        # FIXME: use file(COPY_FILE) if minimum CMake version >= 3.21
        execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E copy_if_different
            \"${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtf-$<CONFIG>.pc\"
            \"${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtf.pc\")
        file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${PC_DESTDIR}\"
            TYPE FILE
            FILES \"${CMAKE_CURRENT_BINARY_DIR}/SDL2_rtf.pc\")" COMPONENT devel)

    if(SDL2RTF_BUILD_SHARED_LIBS AND (APPLE OR (UNIX AND NOT ANDROID)))
        install(FILES
            "${PROJECT_BINARY_DIR}/libSDL2_rtf$<$<CONFIG:Debug>:${SDL2RTF_DEBUG_POSTFIX}>$<TARGET_FILE_SUFFIX:SDL2_rtf>"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT devel
        )
    endif()

    install(FILES "LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
        COMPONENT library
    )
endif()

if(SDL2RTF_SAMPLES)
    add_executable(showrtf showrtf.c)

    find_package(SDL2main QUIET)

    foreach(prog showrtf)
        if(MINGW)
            target_link_libraries(${prog} PRIVATE mingw32)
            target_link_options(${prog} PRIVATE -mwindows)
        endif()
        target_link_libraries(${prog} PRIVATE SDL2_rtf::${sdl2_rtf_export_name})
        if(TARGET SDL2::SDL2main)
            target_link_libraries(${prog} PRIVATE SDL2::SDL2main)
        endif()
        target_link_libraries(${prog} PRIVATE ${sdl2_target_name})
        target_link_libraries(${prog} PRIVATE ${sdl2ttf_target_name})
    endforeach()
endif()
