# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.65)
AC_INIT([pward],[0.6.1],[dirkmoermans@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
AC_CONFIG_SRCDIR([src/proc_impl.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

AC_CHECK_LIB(proc,openproc)

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
