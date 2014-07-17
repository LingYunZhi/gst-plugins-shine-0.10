# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstshinemp3enc.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstshinemp3enc_la_SOURCES = gstshinemp3enc.c gstshinemp3enc.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstshinemp3enc_la_CFLAGS = $(GST_CFLAGS)
libgstshinemp3enc_la_LIBADD = $(GST_LIBS) $(SHINE_LIBS)
libgstshinemp3enc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstshinemp3enc_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstshinemp3enc.h
