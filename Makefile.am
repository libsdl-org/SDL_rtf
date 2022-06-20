# Makefile.am for the SDL Rich Text Format loading library and viewer
ACLOCAL_AMFLAGS = -I acinclude
AUTOMAKE_OPTIONS= foreign

lib_LTLIBRARIES = libSDL2_rtf.la

libSDL2_rtfincludedir = $(includedir)/SDL2
libSDL2_rtfinclude_HEADERS =	\
	SDL_rtf.h

libSDL2_rtf_la_SOURCES =		\
	rtfactn.c		\
	rtfdecl.h		\
	rtfreadr.c		\
	rtftype.h		\
	SDL_rtfreadr.c		\
	SDL_rtfreadr.h		\
	SDL_rtf.c

EXTRA_DIST = CHANGES.txt LICENSE.txt README.txt \
	version.rc		\
	Makefile.os2		\
	SDL2_rtf.spec		\
	SDL2_rtf.spec.in	\
	autogen.sh


libSDL2_rtf_la_LDFLAGS = 	\
	-no-undefined		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) $(LT_EXTRA)
if USE_VERSION_RC
libSDL2_rtf_la_DEPENDENCIES = version.o
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = SDL2_rtf.pc

.rc.o:
	$(RC) $< $@

if HAVE_SDL_TTF
EXAMPLES = showrtf
else
EXAMPLES =
endif
noinst_PROGRAMS = $(EXAMPLES)

showrtf_SOURCES = showrtf.c
showrtf_LDADD = libSDL2_rtf.la @SDLTTF_LIBS@
showrtf_CFLAGS = @SDLTTF_CFLAGS@

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: distcheck

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(libdir)/cmake/SDL2_rtf
	$(INSTALL) -m 644 sdl2_rtf-config.cmake $(DESTDIR)$(libdir)/cmake/SDL2_rtf
	$(INSTALL) -m 644 sdl2_rtf-config-version.cmake $(DESTDIR)$(libdir)/cmake/SDL2_rtf

uninstall-hook:
	rm $(DESTDIR)$(libdir)/cmake/SDL2_rtf/sdl2_rtf-config.cmake
	rm $(DESTDIR)$(libdir)/cmake/SDL2_rtf/sdl2_rtf-config-version.cmake
	rm -r $(DESTDIR)$(libdir)/cmake/SDL2_rtf
