#!/bin/sh
# Copyright 2022 Collabora Ltd.
# SPDX-License-Identifier: Zlib

set -eu

# Needed so sed doesn't report illegal byte sequences on macOS
export LC_CTYPE=C

ref_major=$(sed -ne 's/^#define SDL_RTF_MAJOR_VERSION  *//p' SDL_rtf.h)
ref_minor=$(sed -ne 's/^#define SDL_RTF_MINOR_VERSION  *//p' SDL_rtf.h)
ref_micro=$(sed -ne 's/^#define SDL_RTF_PATCHLEVEL  *//p' SDL_rtf.h)
ref_version="${ref_major}.${ref_minor}.${ref_micro}"
ref_sdl_req=$(sed -ne 's/^SDL_VERSION=//p' configure.ac)

tests=0
failed=0

ok () {
    tests=$(( tests + 1 ))
    echo "ok - $*"
}

not_ok () {
    tests=$(( tests + 1 ))
    echo "not ok - $*"
    failed=1
}

major=$(sed -Ene 's/^m4_define\(\[MAJOR_VERSION_MACRO\], \[([0-9]*)\]\)$/\1/p' configure.ac)
minor=$(sed -Ene 's/^m4_define\(\[MINOR_VERSION_MACRO\], \[([0-9]*)\]\)$/\1/p' configure.ac)
micro=$(sed -Ene 's/^m4_define\(\[MICRO_VERSION_MACRO\], \[([0-9]*)\]\)$/\1/p' configure.ac)
version="${major}.${minor}.${micro}"

if [ "$ref_version" = "$version" ]; then
    ok "configure.ac $version"
else
    not_ok "configure.ac $version disagrees with SDL_ttf.h $ref_version"
fi

major=$(sed -ne 's/^set(MAJOR_VERSION \([0-9]*\))$/\1/p' CMakeLists.txt)
minor=$(sed -ne 's/^set(MINOR_VERSION \([0-9]*\))$/\1/p' CMakeLists.txt)
micro=$(sed -ne 's/^set(MICRO_VERSION \([0-9]*\))$/\1/p' CMakeLists.txt)
sdl_req=$(sed -ne 's/^set(SDL_REQUIRED_VERSION \([0-9.]*\))$/\1/p' CMakeLists.txt)
version="${major}.${minor}.${micro}"

if [ "$ref_version" = "$version" ]; then
    ok "CMakeLists.txt $version"
else
    not_ok "CMakeLists.txt $version disagrees with SDL_ttf.h $ref_version"
fi

if [ "$ref_sdl_req" = "$sdl_req" ]; then
    ok "CMakeLists.txt $sdl_req"
else
    not_ok "CMakeLists.txt SDL_REQUIRED_VERSION=$sdl_req disagrees with configure.ac SDL_VERSION=$ref_sdl_req"
fi

major=$(sed -ne 's/^MAJOR_VERSION *= *//p' Makefile.os2)
minor=$(sed -ne 's/^MINOR_VERSION *= *//p' Makefile.os2)
micro=$(sed -ne 's/^MICRO_VERSION *= *//p' Makefile.os2)
version="${major}.${minor}.${micro}"

if [ "$ref_version" = "$version" ]; then
    ok "Makefile.os2 $version"
else
    not_ok "Makefile.os2 $version disagrees with SDL_ttf.h $ref_version"
fi

for rcfile in version.rc; do
    tuple=$(sed -ne 's/^ *FILEVERSION *//p' "$rcfile" | tr -d '\r')
    ref_tuple="${ref_major},${ref_minor},${ref_micro},0"

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile FILEVERSION $tuple"
    else
        not_ok "$rcfile FILEVERSION $tuple disagrees with SDL_ttf.h $ref_tuple"
    fi

    tuple=$(sed -ne 's/^ *PRODUCTVERSION *//p' "$rcfile" | tr -d '\r')

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile PRODUCTVERSION $tuple"
    else
        not_ok "$rcfile PRODUCTVERSION $tuple disagrees with SDL_ttf.h $ref_tuple"
    fi

    tuple=$(sed -Ene 's/^ *VALUE "FileVersion", "([0-9, ]*)\\0"\r?$/\1/p' "$rcfile" | tr -d '\r')
    ref_tuple="${ref_major}, ${ref_minor}, ${ref_micro}, 0"

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile FileVersion $tuple"
    else
        not_ok "$rcfile FileVersion $tuple disagrees with SDL_ttf.h $ref_tuple"
    fi

    tuple=$(sed -Ene 's/^ *VALUE "ProductVersion", "([0-9, ]*)\\0"\r?$/\1/p' "$rcfile" | tr -d '\r')

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile ProductVersion $tuple"
    else
        not_ok "$rcfile ProductVersion $tuple disagrees with SDL_ttf.h $ref_tuple"
    fi
done

#sdl_req=$(sed -ne 's/\$sdl2_version = "\([0-9.]*\)"$/\1/p' .github/fetch_sdl_vc.ps1)
#
#if [ "$ref_sdl_req" = "$sdl_req" ]; then
#    ok ".github/fetch_sdl_vc.ps1 $sdl_req"
#else
#    not_ok ".github/fetch_sdl_vc.ps1 sdl2_version=$sdl_req disagrees with configure.ac SDL_VERSION=$ref_sdl_req"
#fi

echo "1..$tests"
exit "$failed"
