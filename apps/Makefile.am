## Process this file with automake to produce Makefile.in
# *************************************************************************
# * GSM TA/ME library
# *
# * File:    Makefile.am
# *
# * Purpose: apps Makefile
# *
# * Author:  Peter Hofmann (software@pxh.de)
# *
# * Created: 5.6.1999
# *************************************************************************

INCLUDES =		-I..

bin_PROGRAMS =		gsmsmsstore gsmctl gsmsmsd gsmpb gsmsendsms

# build gsmsmsd from gsmsmsd.cc and libgsm.la
gsmsmsd_SOURCES =	gsmsmsd.cc
gsmsmsd_LDADD =		../gsmlib/libgsm.la $(INTLLIBS)

# build gsmpb from gsmpb.cc and libgsm.la
gsmpb_SOURCES =		gsmpb.cc
gsmpb_LDADD =		../gsmlib/libgsm.la $(INTLLIBS)

# build gsmctl from gsmctl.cc and libgsm.la
gsmctl_SOURCES =	gsmctl.cc
gsmctl_LDADD =		../gsmlib/libgsm.la $(INTLLIBS)

# build gsmsendsms from gsmsendsms.cc and libgsm.la
gsmsendsms_SOURCES =	gsmsendsms.cc
gsmsendsms_LDADD =	../gsmlib/libgsm.la $(INTLLIBS)

# build gsmsmsstore from gsmsmsstore.cc and libgsm.la
gsmsmsstore_SOURCES =	gsmsmsstore.cc
gsmsmsstore_LDADD =	../gsmlib/libgsm.la $(INTLLIBS)
