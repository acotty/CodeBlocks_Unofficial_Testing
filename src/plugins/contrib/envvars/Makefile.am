.rc.lo:
	$(LIBTOOL) --tag=RC --mode=compile $(WX_RESCOMP) $< -o $@

AM_CPPFLAGS = \
			$(WX_CXXFLAGS) \
			$(CB_SQUIRREL_CFLAGS) \
			-I$(top_srcdir)/src/include \
			-I$(top_srcdir)/src/sdk/wxscintilla/include

if CODEBLOCKS_NT_BUILD_INSTALLER
pluginlibdir = $(pkgdatadir)/plugins
else
pluginlibdir = $(pkglibdir)/plugins
endif

pkgdata_DATA = \
			envvars.zip

pluginlib_LTLIBRARIES = \
			libenvvars.la

libenvvars_la_LDFLAGS = \
			@MODULE_SHARED_LDFLAGS@ \
			-no-undefined \
			-avoid-version

libenvvars_la_LIBADD = \
			$(top_srcdir)/src/sdk/libcodeblocks.la \
			$(WX_LIBS)

if !CODEBLOCKS_NT
libenvvars_la_LIBADD += \
			$(CB_TINYXML_LIBS)
endif

libenvvars_la_SOURCES = \
			envvars.cpp \
			envvars_cfgdlg.cpp \
			envvars_common.cpp \
			envvars_prjoptdlg.cpp

if CODEBLOCKS_NT
libenvvars_la_SOURCES += \
			manifest.rc
endif

noinst_HEADERS = \
			envvars.h \
			envvars_cfgdlg.h \
			envvars_common.h \
			envvars_prjoptdlg.h

EXTRA_DIST = \
			*.cbp \
			envvars.xrc \
			project_settings.xrc \
			manifest.xml

CLEANFILES = \
			$(pkgdata_DATA)

envvars.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/envvars.zip manifest.xml *.xrc > /dev/null
