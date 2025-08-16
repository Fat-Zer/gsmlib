dnl *************************************************************************
dnl * GSM TA/ME library
dnl *
dnl * File:    configure.in
dnl *
dnl * Purpose: autoconf configure script template
dnl *
dnl * Author:  Peter Hofmann (software@pxh.de)
dnl *
dnl * Created: 11.11.1999
dnl *************************************************************************

dnl project name and version
AC_INIT([gsmlib],[1.11])

dnl Other
AC_CONFIG_AUX_DIR(scripts)
AC_PROG_INSTALL

dnl check for libintl
AC_CHECK_LIB(intl, textdomain)

dnl use config header
AC_CONFIG_HEADERS(gsm_config.h)

dnl automake options; foreign to prevent autotools from rewriting INSTALL
AM_INIT_AUTOMAKE([foreign])

dnl change to no if you want no shared libraries for debugging purposes
AC_ENABLE_SHARED([yes])

dnl use -O2 optimization by default
if test "$CXXFLAGS" = ""; then
   CXXFLAGS="-O2"
fi

dnl comment out this line to get extensive debugging output and asserts
dnl CXXFLAGS="-DNDEBUG $CXXFLAGS"

dnl uncomment to get translations without installing gsmlib
dnl CXXFLAGS="-DLOCAL_TRANSLATIONS $CXXFLAGS"

dnl check _REENTRANT in header files
if test x"`egrep _REENTRANT /usr/include/features.h`" != x; then
  CXXFLAGS="-D_REENTRANT $CXXFLAGS"
  CFLAGS="-D_REENTRANT $CFLAGS"
fi

dnl output all warnings
CXXFLAGS="-Wall $CXXFLAGS"

dnl use libtool
LT_INIT

dnl Checks for programs.
AC_PROG_CPP
AC_PROG_CXX

dnl check for gcc 2.95.x
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
int main()
{
#if defined(__GNUC__) && \
    ! (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
    return 1;
#endif
  return 0;
}
]])],[],[echo "need at least gcc 2.95 to compile correctly"
exit 1],[])

dnl check for alloca
AC_FUNC_ALLOCA

dnl check for getopt_long in the C library
AC_CHECK_LIB(c, getopt_long, AC_DEFINE(HAVE_GETOPT_LONG))

dnl check for alarm in the C library
AC_CHECK_LIB(c, alarm, AC_DEFINE(HAVE_ALARM))

dnl check for netinet/in.h header
AC_CHECK_HEADERS(netinet/in.h)

dnl check for string.h header
AC_CHECK_HEADERS(string.h)

dnl check for libintl.h header
AC_CHECK_HEADERS(libintl.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl check for vsnprintf()
dnl AC_FUNC_VPRINTF
AC_EGREP_HEADER(vsnprintf, stdio.h, AC_DEFINE(HAVE_VSNPRINTF))

dnl checks for builtin data type sizes
AC_CHECK_SIZEOF(unsigned short int, 2)
AC_CHECK_SIZEOF(unsigned long int, 4)
AC_CHECK_SIZEOF(unsigned int, 4)

dnl Project-specific settings
GSM_VERSION="1:5:0"
AC_SUBST(GSM_VERSION)

dnl national language support (NLS)
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.20)
ALL_LINGUAS=`cd $srcdir/po > /dev/null && echo *.po | sed 's/\.po//g'`

dnl set locale dir
AC_DEFINE_UNQUOTED([LOCALEDIR], [$localedir_c], [Install directory for localizations])

dnl check system funcs
AC_CHECK_FUNCS([alarm vsnprintf getopt_long])

AC_CONFIG_FILES([Makefile gsmlib/Makefile tests/Makefile apps/Makefile win32/Makefile
          doc/Makefile scripts/Makefile po/Makefile.in
          ext/Makefile contrib/Makefile])
AC_CONFIG_COMMANDS([default],[echo timestamp > stamp-h],[])
AC_OUTPUT
