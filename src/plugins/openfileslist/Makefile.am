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
			libopenfileslist.la

libopenfileslist_la_LDFLAGS = \
			@MODULE_SHARED_LDFLAGS@ \
			-no-undefined \
			-avoid-version

libopenfileslist_la_LIBADD = \
			$(top_srcdir)/src/sdk/libcodeblocks.la \
			$(WX_LIBS)

if !CODEBLOCKS_NT
libopenfileslist_la_LIBADD += \
			$(CB_TINYXML_LIBS)
endif

libopenfileslist_la_SOURCES = \
			openfileslistplugin.cpp

if CODEBLOCKS_NT
libopenfileslist_la_SOURCES += \
			manifest.rc
endif

noinst_HEADERS = \
			openfileslistplugin.h

pkgdata_DATA = \
			openfileslist.zip

CLEANFILES = \
			$(pkgdata_DATA)

EXTRA_DIST = \
			manifest.xml

openfileslist.zip: $(EXTRA_DIST)
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/openfileslist.zip manifest.xml > /dev/null

