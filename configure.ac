#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([udtgate], [1.3-fix1], [vkonoplev@acm.org])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
AC_PROG_RANLIB
AC_LANG_CPLUSPLUS
AC_CONFIG_SRCDIR([src/lib/socket_api.cpp])
AC_CONFIG_HEADER([src/include/config.h])

# Check for OS type
case "$target" in
*-linux-*)
        OS=LINUX
        ;;
*-sunos*)
        OS=SUNOS
        ;;
*-*-irix*)
        OS=IRIX
        ;;
*-*-solaris*)
        OS=SOLARIS
        ;;
*-freebsd*)
        OS=FREEBSD
        ;;
*dec-os*)
        OS=ALPHA
        ;;
*)
        OS=UNIX
        ;;
esac

CXXFLAGS="$CXXFLAGS -DOS_$OS"

# Debug option processing
AC_ARG_ENABLE(debug,
	     AC_HELP_STRING([--enable-debug], [enables debug options]),
	     [CXXFLAGS="$CXXFLAGS -ggdb3 -O0"]
)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_PROG_GCC_TRADITIONAL

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread kse], ,
    AC_MSG_FAILURE([Can not find mandatory pthread library.]))

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h sys/socket.h sys/time.h \ 
                  syslog.h unistd.h sys/ioctl.h limits.h sys/fs/s5param.h \
                  sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_TYPE_OFF_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_GETPGRP
AC_FUNC_SETPGRP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_HEADER_SYS_WAIT

AC_CHECK_FUNCS([gettimeofday inet_ntoa memset pow select socket strcasecmp \
                    strerror strdup getifaddrs])

# UDT scpecific checks
m4_include([checkudt.m4])

#

# Well, GO!

AC_CONFIG_FILES([Makefile
                 src/lib/Makefile
                 src/relay/Makefile
                 src/gate/Makefile
                 src/test/Makefile
                 ])
AC_OUTPUT
