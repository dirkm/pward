# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.65)
AC_INIT([pward],[0.7.2],[dirkmoermans@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_SRCDIR([src/proc_impl.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

AC_CHECK_LIB(proc,get_proc_stats,,[AC_MSG_ERROR([dynamic libproc not found])])

# TODO: temporary workaround to get symbol read_cmdline
#  read_cmdline is not part of the endorsed interface and only defined in the static lib
#  thus we compile against the static and dynamic library, to have all needed symbols

LIBPROC_STATIC=/usr/lib/libproc.a
AC_CHECK_FILE($LIBPROC_STATIC,,[AC_MSG_ERROR([static libproc not found])])
AC_SUBST(LIBPROC_STATIC)


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h proc/readproc.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([strtol])

AC_OUTPUT
