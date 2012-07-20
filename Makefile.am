## Process this file with automake to produce Makefile.in


pcfiles = libccnet2.pc
pkgconfig_DATA = $(pcfiles)
pkgconfigdir = $(libdir)/pkgconfig

MAKE_CLI = cli
MAKE_TOOLS = tools

SUBDIRS = include lib net $(MAKE_CLI) $(MAKE_TOOLS) \
	python

EXTRA_DIST = config.rpath install-sh
EXTRA_DIST += libccnet2.pc.in

ACLOCAL_AMFLAGS = -I m4
