.rc.lo:
	$(LIBTOOL) --tag=RC --mode=compile $(WX_RESCOMP) $< -o $@

AM_CPPFLAGS = \
			$(WX_CXXFLAGS) \
			-I$(top_srcdir)/src/include \
			-I$(top_srcdir)/src/sdk/wxscintilla/include

if CODEBLOCKS_NT_BUILD_INSTALLER
pluginlibdir = $(pkgdatadir)/plugins
else
pluginlibdir = $(pkglibdir)/plugins
endif

pluginlib_LTLIBRARIES = \
			libbyogames.la

libbyogames_la_LDFLAGS = \
			@MODULE_SHARED_LDFLAGS@ \
			-no-undefined \
			-avoid-version

libbyogames_la_LIBADD = \
			$(top_srcdir)/src/sdk/libcodeblocks.la $(WX_LIBS)

libbyogames_la_SOURCES = \
			byocbtris.cpp \
			byogame.cpp \
			byogameselect.cpp \
			byogames.cpp \
			byogamebase.cpp \
			byoconf.cpp \
			byosnake.cpp

if CODEBLOCKS_NT
libbyogames_la_SOURCES += \
			manifest.rc
endif

noinst_HEADERS = \
			byogameselect.h \
			byocbtris.h \
			byogame.h \
			byogames.h \
			byogamebase.h \
			byoconf.h \
			byosnake.h

EXTRA_DIST = \
			wxsmith/*.wxs \
			*.cbp \
			manifest.xml

pkgdata_DATA = \
			byogames.zip
CLEANFILES = \
			$(pkgdata_DATA)

byogames.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/byogames.zip manifest.xml > /dev/null
