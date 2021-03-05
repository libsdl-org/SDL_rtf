dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_SRCDIR([SDL_rtf.c])

dnl Set various version strings - taken gratefully from the GTk sources

# Making releases:
#   MICRO_VERSION += 1;
#   INTERFACE_AGE += 1;
#   BINARY_AGE += 1;
# if any functions have been added, set INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set BINARY_AGE and INTERFACE_AGE to 0.

MAJOR_VERSION=2
MINOR_VERSION=0
MICRO_VERSION=0
INTERFACE_AGE=0
BINARY_AGE=0
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(INTERFACE_AGE)
AC_SUBST(BINARY_AGE)
AC_SUBST(VERSION)

# libtool versioning
LT_INIT([win32-dll])

LT_RELEASE=$MAJOR_VERSION.$MINOR_VERSION
LT_CURRENT=`expr $MICRO_VERSION - $INTERFACE_AGE`
LT_REVISION=$INTERFACE_AGE
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl Detect the canonical build and host environments
AC_CANONICAL_HOST

dnl Setup for automake
AM_INIT_AUTOMAKE(SDL_rtf, $VERSION)

dnl Check for tools
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
if test -z "$host_alias"; then
    hostaliaswindres=
else
    hostaliaswindres="$host_alias-windres"
fi
AC_CHECK_PROGS(WINDRES, [windres $hostaliaswindres $host_os-windres])

case "$host" in
    *-*-beos*)
        ac_default_prefix=/boot/develop/tools/gnupro
        ;;
    *-*-cygwin* | *-*-mingw*)
        if test "$build" != "$host"; then # cross-compiling
            # Default cross-compile location
            ac_default_prefix=/usr/local/cross-tools/$host
        else
            # Look for the location of the tools and install there
            if test "$BUILD_PREFIX" != ""; then
                ac_default_prefix=$BUILD_PREFIX
            fi
        fi
        if test x$WINDRES != x; then
            use_version_rc=true
        fi
        ;;
esac
AM_CONDITIONAL(USE_VERSION_RC, test x$use_version_rc = xtrue)

dnl Check for SDL
SDL_VERSION=2.0.0
AM_PATH_SDL2($SDL_VERSION,
            :,
	    AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

dnl Check for SDL_ttf
have_SDL_ttf=no
AC_ARG_ENABLE([SDL_ttf], [AS_HELP_STRING([--enable-SDL_ttf], [Try to build the showrtf test program [default=yes]])],
              [], [enable_SDL_ttf=yes])
if test x$enable_SDL_ttf = xyes; then
    PKG_CHECK_MODULES([SDLTTF], [SDL2_ttf], [have_SDL_ttf=yes], [dnl
        AC_CHECK_LIB([SDL2_ttf],[TTF_Init],[have_SDL_ttf=yes; SDLTTF_LIBS="-lSDL2_ttf"],[],[])
    ])
    if test x$have_SDL_ttf != xyes; then
        AC_MSG_WARN([*** Can't find the SDL2_ttf library - not building showrtf
The SDL2_ttf library can be found at:
http://www.libsdl.org/projects/SDL_ttf/
])
    fi
fi
AM_CONDITIONAL(HAVE_SDL_TTF, test x$have_SDL_ttf = xyes)

AC_SUBST([WINDRES])

# Finally create all the generated files
AC_CONFIG_FILES([
Makefile
SDL_rtf.spec
])
AC_OUTPUT
