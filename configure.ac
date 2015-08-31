#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(broadcasttalk, 0.1, mykeich@yandex.ru)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/broadcasttalk.c])
# AC_CONFIG_HEADERS([broadcasttalk.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h sys/time.h unistd.h utime.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset socket strrchr])

AC_PROG_RANLIB
AC_OUTPUT(Makefile src/Makefile src/gsm/Makefile)

AC_CHECK_LIB([pthread],[asound])



