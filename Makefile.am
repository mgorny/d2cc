toolchaindir = $(libexecdir)/d2cc/toolchain
d2ccincludedir = $(includedir)/libd2cc

ACLOCAL_AMFLAGS = -I build-aux
AM_CXXFLAGS = -std=c++11

d2ccinclude_HEADERS = \
	src/libd2cc/protocol.h \
	src/libd2cc/serialization.h \
	src/libd2cc/unixclient.h

lib_LTLIBRARIES = lib/libd2cc.la
lib_libd2cc_la_CPPFLAGS = \
	-I $(top_srcdir)/src \
	-DD2CC_SOCKET_PATH='"$(localstatedir)/d2cc.sock"'
lib_libd2cc_la_SOURCES = \
	src/libd2cc/protocol.cxx \
	src/libd2cc/unixclient.cxx \
	$(d2ccinclude_HEADERS)

bin_PROGRAMS = bin/d2ccd
bin_d2ccd_SOURCES = \
	src/d2ccd/d2ccd.cxx
bin_d2ccd_CPPFLAGS = \
	$(lib_libd2cc_la_CPPFLAGS)
bin_d2ccd_LDADD = \
	lib/libd2cc.la

toolchain_PROGRAMS = toolchain/gcc
toolchain_gcc_SOURCES = \
	src/toolchain/gcc.cxx
toolchain_gcc_CPPFLAGS = \
	$(lib_libd2cc_la_CPPFLAGS)
toolchain_gcc_LDADD = \
	lib/libd2cc.la
