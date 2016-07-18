# This file was automatically generated
# DO NOT EDIT!
#
lib_LTLIBRARIES = libsndfile.la
include_HEADERS = sndfile.h
INCLUDES = -I$(srcdir)/../src -I$(srcdir)

noinst_HEADERS = common.h sfendian.h pcm.h ulaw.h alaw.h wav.h

COMMON = common.c pcm.c ulaw.c alaw.c sndfile.c 
FILESPECIFIC = aiff.c au.c wav.c wav_float.c wav_ima_adpcm.c wav_ms_adpcm.c raw.c

libsndfile_la_SOURCES = $(FILESPECIFIC) $(COMMON) sndfile.h common.h
libsndfile_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@

# Disable autoheader. 
AUTOHEADER=echo
