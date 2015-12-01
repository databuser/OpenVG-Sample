#-----------------------------------------------------------------------------
# Copyright (c) 2008-2009 Intel Corporation
#
# DISTRIBUTABLE AS SAMPLE SOURCE SOFTWARE
#
# This Distributable As Sample Source Software is subject to the terms and
# conditions of the Intel Software License Agreement provided with the Intel(R)
# Media Processor Software Development Kit.
#------------------------------------------------------------------------------

# To use this makefile you must uncomment the following line and make
# sure the specified binaries are in your PATH:
#
# TARGETCC   = i686-cm-linux-gcc
#
# You must also define (in the environment, on the make invocation line, or in
# an including makefile) the following make variables,
#
# INCLUDE_DIRS	directory path(s) to .h files like libgdl.h, gdl.h
# LIB_DIRS		directory path(s) to libgdl.so libosal.so
#
#11 More than one path (separated by spaces) can be specified for each
INCLUDE_DIRS=/home/sunsik/Project/Android-POC/IntelCE/build_i686/staging_dir/include/
LIB_DIRS=/home/sunsik/Project/Android-POC/IntelCE/build_i686/staging_dir/lib/
BIN_DIRS=/home/sunsik/Project/Android-POC/IntelCE/build_i686/staging_dir/bin/

#ifndef INCLUDE_DIRS
#$(error INCLUDE_DIRS not specified)
#endif

#ifndef LIB_DIRS
#$(error LIB_DIRS not specified)
#endif


# If an application links against an .so (SO1) which is in turn linked against a
# 2nd .so (SO2), the link will fail if SO2 can't be found at link time.  This
# can be fixed by making sure SO2 can be found in either:
#    - the current LD_LIBRARY_PATH, or 
#    - a path in a linker -rpath option
# But it's unreasonable to expect the app writer to know all the dependencies
# of SO1 and where those files are at compile time.  The following option will
# cause the linker to ignore unresolved references from SO1 rather than failing.
SO_WORKAROUND ?= --unresolved-symbols=ignore-in-shared-libs

INCLUDES=-I. $(foreach d,$(INCLUDE_DIRS),-I$d)
LIBS	= -Wl,$(SO_WORKAROUND) \
          $(foreach d,$(LIB_DIRS),-L$d -Wl,-rpath,$d) \
          -lgdl \
          -lgma \
          -lGLES_CM \
          -lEGL \
	  -lOpenVG \
	  -lOpenVGU	
		

#TARGETCC ?= $(BUILD_DEST)/bin/i686-cm-linux-gcc
TARGETCC ?= $(BIN_DIRS)/i686-cm-linux-gcc

SAMPLE = main

all : $(SAMPLE)

$(SAMPLE) : $(SAMPLE).o  tiger.o
	$(TARGETCC) $(CFLAGS) -o $@ $^ $(LIBS)

.c.o:
	$(TARGETCC) -c $(CFLAGS) $(INCLUDES) $< -o $(@D)/$(@F)

debug : all

clean :
	$(RM) *.o $(SAMPLE) 
